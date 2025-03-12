/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#define LOG_TAG "selinux"

#include "selinux.h"

#include <dirent.h>
#include <minimal_systems/log.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>

#include "boot_clock.h"
#include "fs_mgr.h"
#include "log_new.h"
#include "property_manager.h"
#include "util.h"

namespace minimal_systems
{
namespace init
{

// Hardened SELinux directories for major Linux distributions
static const std::vector<std::string> kSelinuxWhitelist = {
	"/etc/selinux", // Standard SELinux config
	"/usr/share/selinux", // Common SELinux policy storage
	"/usr/etc/selinux", // Some distributions use this
	"/lib/selinux", // SELinux modules and libraries
	"/run/selinux", // Runtime SELinux directory
};

std::unique_ptr<SELinuxEntry> selinux_entries_head;
bool selinux_disabled_permanently = false;

/**
 * Determine SELinux enforcing status from system properties.
 */
EnforcingStatus StatusFromProperty()
{
	std::string value;
	if (minimal_systems::fs_mgr::GetKernelCmdline("sysboot.selinux",
						      &value) &&
	    value == "permissive") {
		return SELINUX_PERMISSIVE;
	}
	if (minimal_systems::fs_mgr::GetBootconfig("sysboot.selinux", &value) &&
	    value == "permissive") {
		return SELINUX_PERMISSIVE;
	}
	return SELINUX_ENFORCING;
}

/**
 * Check if SELinux is enforcing, considering overrides.
 */
bool IsEnforcing()
{
	if (selinux_disabled_permanently) {
		return false;
	}
#ifdef ALLOW_PERMISSIVE_SELINUX
	return StatusFromProperty() == SELINUX_ENFORCING;
#else
	return true;
#endif
}

/**
 * Check if a given path is whitelisted for SELinux policy scanning.
 */
bool IsWhitelistedPath(const std::string &path)
{
	return std::find(kSelinuxWhitelist.begin(), kSelinuxWhitelist.end(),
			 path) != kSelinuxWhitelist.end();
}

/**
 * Store SELinux entries for debugging and policy analysis.
 */
void StoreSELinuxEntry(const std::string &entry)
{
	auto new_entry = std::make_unique<SELinuxEntry>();
	new_entry->entry = entry;
	new_entry->next = std::move(selinux_entries_head);
	selinux_entries_head = std::move(new_entry);
}

/**
 * Parse the SELinux configuration file and set system properties.
 */
bool ParseSELinuxConfig(const std::string &filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		LOGE("Error: Unable to open SELinux configuration file at '%s'.",
		     filepath.c_str());
		return false;
	}

	auto &props = PropertyManager::instance();
	std::string line, selinux_state = "unknown", selinux_type = "unknown";

	LOGI("Parsing SELinux configuration from '%s'.", filepath.c_str());
	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t"));

		if (line.empty() || line[0] == '#')
			continue;

		if (line.find("SELINUX=") == 0) {
			selinux_state = line.substr(8);
		} else if (line.find("SELINUXTYPE=") == 0) {
			selinux_type = line.substr(12);
		}
	}

	props.set("ro.boot.selinux",
		  selinux_state == "disabled" ? "permissive" : selinux_state);
	props.set("ro.boot.selinux_type", selinux_type);

	LOGI("SELinux state set to '%s'.", selinux_state.c_str());
	LOGI("SELinux policy type set to '%s'.", selinux_type.c_str());

	return true;
}

/**
 * Parse a SELinux policy file and store valid entries.
 */
bool ParseSELinuxFile(const std::string &filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		LOGE("Error: Failed to open SELinux policy file '%s'.",
		     filepath.c_str());
		return false;
	}

	std::string line;
	LOGI("Processing SELinux policy file: '%s'.", filepath.c_str());
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#' ||
		    line.find("system_u:object_") == std::string::npos) {
			continue;
		}
		StoreSELinuxEntry(line);
	}

#if !LOG_NDEBUG
	LOGD("Debug: Finished parsing SELinux policy file '%s'.",
	     filepath.c_str());
#endif
	return true;
}

/**
 * Traverse a given directory for SELinux policy files.
 */
bool TraverseAndParse(const std::string &dir_path)
{
	DIR *dir = opendir(dir_path.c_str());
	if (!dir) {
		LOGE("Error: Unable to access SELinux directory '%s'.",
		     dir_path.c_str());
		return false;
	}

	struct dirent *entry;
	bool found_policy = false;

	while ((entry = readdir(dir)) != nullptr) {
		if (std::strcmp(entry->d_name, ".") == 0 ||
		    std::strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		std::string full_path = dir_path + "/" + entry->d_name;

		if (entry->d_type == DT_REG) {
			if (ParseSELinuxFile(full_path)) {
				found_policy = true;
			}
		}
	}

	closedir(dir);
	return found_policy;
}

/**
 * Initialize SELinux and apply policy settings.
 */
int SetupSelinux(char **argv)
{
	SetStdioToDevNull(argv);
	InitKernelLogging(argv);

	auto &props = PropertyManager::instance();
	LOGI("Initializing SELinux setup...");

	bool config_loaded = ParseSELinuxConfig("/etc/selinux/config");
	bool policy_loaded = false;

	for (const auto &selinux_path : kSelinuxWhitelist) {
		LOGI("Scanning SELinux policy directory: '%s'.",
		     selinux_path.c_str());
		if (TraverseAndParse(selinux_path)) {
			policy_loaded = true;
		}
	}

	if (!config_loaded || !policy_loaded) {
		LOGE("Critical: No valid SELinux policies or configuration files found. "
		     "Disabling SELinux permanently.");
		selinux_disabled_permanently = true;
		props.set("ro.boot.selinux", "permissive");
		return 0;
	}

	std::string selinux_mode = props.get("ro.boot.selinux", "enforcing");
	LOGI("SELinux mode is set to '%s'.", selinux_mode.c_str());

#if !LOG_NDEBUG
	LOGD("Debug: SELinux initialization complete.");
#endif

	return 1;
}

} // namespace init
} // namespace minimal_systems
