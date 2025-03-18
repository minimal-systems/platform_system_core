/*
 * Copyright (C) 2024 Open Source Community
 *
 * Licensed under the GNU General Public License, Version 2.0 (GPLv2)
 */

#include "modprobe.h"
#include "exthandler.h"
#include "log_new.h"

#include <dirent.h>
#include <fnmatch.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

std::string Modprobe::MakeCanonical(const std::string& module_path) {
    auto start = module_path.find_last_of('/');
    if (start == std::string::npos) {
        start = 0;
    } else {
        start += 1;
    }
    auto end = module_path.size();
    if (module_path.rfind(".ko") == (module_path.size() - 3)) {
        end -= 3;
    }
    if ((end - start) <= 1) {
        LOGE("Malformed module name: %s", module_path.c_str());
        return "";
    }
    std::string module_name = module_path.substr(start, end - start);
    std::replace(module_name.begin(), module_name.end(), '-', '_');
    return module_name;
}

bool Modprobe::ParseDepCallback(const std::string& base_path,
                                const std::vector<std::string>& args) {
    if (args.empty()) return false;

    std::vector<std::string> deps;
    std::string prefix = "";

    size_t pos = args[0].find(':');
    if (args[0][0] != '/') {
        prefix = base_path + "/";
    }
    if (pos != std::string::npos) {
        deps.emplace_back(prefix + args[0].substr(0, pos));
    } else {
        LOGE("Dependency lines must start with name followed by ':'");
        return false;
    }

    for (auto arg = args.begin() + 1; arg != args.end(); ++arg) {
        if ((*arg)[0] != '/') {
            prefix = base_path + "/";
        } else {
            prefix = "";
        }
        deps.push_back(prefix + *arg);
    }

    std::string canonical_name = MakeCanonical(args[0].substr(0, pos));
    if (canonical_name.empty()) {
        return false;
    }
    this->module_deps_[canonical_name] = deps;

    return true;
}

bool Modprobe::ParseAliasCallback(const std::vector<std::string>& args) {
    auto it = args.begin();
    const std::string& type = *it++;

    if (type != "alias") {
        LOGE("Non-alias line encountered in modules.alias, found: %s", type.c_str());
        return false;
    }

    if (args.size() != 3) {
        LOGE("Alias lines in modules.alias must have 3 entries, not %zu", args.size());
        return false;
    }

    const std::string& alias = *it++;
    const std::string& module_name = *it++;
    this->module_aliases_.emplace_back(alias, module_name);

    return true;
}

bool Modprobe::ParseSoftdepCallback(const std::vector<std::string>& args) {
    auto it = args.begin();
    const std::string& type = *it++;
    std::string state = "";

    if (type != "softdep") {
        LOGE("Non-softdep line encountered in modules.softdep, found: %s", type.c_str());
        return false;
    }

    if (args.size() < 4) {
        LOGE("Softdep lines in modules.softdep must have at least 4 entries, not %zu", args.size());
        return false;
    }

    const std::string& module = *it++;
    while (it != args.end()) {
        const std::string& token = *it++;
        if (token == "pre:" || token == "post:") {
            state = token;
            continue;
        }
        if (state.empty()) {
            LOGE("Malformed modules.softdep at token: %s", token.c_str());
            return false;
        }
        if (state == "pre:") {
            this->module_pre_softdep_.emplace_back(module, token);
        } else {
            this->module_post_softdep_.emplace_back(module, token);
        }
    }

    return true;
}

bool Modprobe::ParseLoadCallback(const std::vector<std::string>& args) {
    auto it = args.begin();
    const std::string& module = *it++;

    const std::string& canonical_name = MakeCanonical(module);
    if (canonical_name.empty()) {
        return false;
    }
    this->module_load_.emplace_back(canonical_name);

    return true;
}

bool Modprobe::ParseOptionsCallback(const std::vector<std::string>& args) {
    auto it = args.begin();
    const std::string& type = *it++;

    if (type == "dyn_options") {
        return ParseDynOptionsCallback(std::vector<std::string>(it, args.end()));
    }

    if (type != "options") {
        LOGE("Non-options line encountered in modules.options");
        return false;
    }

    if (args.size() < 2) {
        LOGE("Lines in modules.options must have at least 2 entries, not %zu", args.size());
        return false;
    }

    const std::string& module = *it++;
    std::string options;

    const std::string& canonical_name = MakeCanonical(module);
    if (canonical_name.empty()) {
        return false;
    }

    while (it != args.end()) {
        options += *it++;
        if (it != args.end()) {
            options += " ";
        }
    }

    auto [unused, inserted] = this->module_options_.emplace(canonical_name, options);
    if (!inserted) {
        LOGE("Multiple options lines present for module %s", module.c_str());
        return false;
    }
    return true;
}

bool Modprobe::ParseDynOptionsCallback(const std::vector<std::string>& args) {
    auto it = args.begin();

    if (args.size() < 3) {
        LOGE("dyn_options lines in modules.options must have at least 3 entries, not %zu",
             args.size());
        return false;
    }

    const std::string& module = *it++;

    const std::string& canonical_name = MakeCanonical(module);
    if (canonical_name.empty()) {
        return false;
    }

    const std::string& pwnam = *it++;
    passwd* pwd = getpwnam(pwnam.c_str());
    if (!pwd) {
        LOGE("Invalid handler UID: %s", pwnam.c_str());
        return false;
    }

    std::string handler_with_args;
    for (; it != args.end(); ++it) {
        handler_with_args += *it + " ";
    }
    handler_with_args.erase(std::remove(handler_with_args.begin(), handler_with_args.end(), '"'),
                            handler_with_args.end());

    LOGD("Launching external module options handler: '%s' for module: %s",
         handler_with_args.c_str(), module.c_str());

    std::unordered_map<std::string, std::string> envs_map;
    std::string result = RunExternalHandler(handler_with_args, pwd->pw_uid, 0, envs_map);
    if (result.empty()) {
        LOGE("External module handler failed");
        return false;
    }

    LOGI("Dynamic options for module %s are '%s'", module.c_str(), result.c_str());

    auto [unused, inserted] = this->module_options_.emplace(canonical_name, result);
    if (!inserted) {
        LOGE("Multiple options lines present for module %s", module.c_str());
        return false;
    }
    return true;
}

bool Modprobe::ParseBlocklistCallback(const std::vector<std::string>& args) {
    auto it = args.begin();
    const std::string& type = *it++;

    if (type != "blocklist") {
        LOGE("Non-blocklist line encountered in modules.blocklist");
        return false;
    }

    if (args.size() != 2) {
        LOGE("Lines in modules.blocklist must have exactly 2 entries, not %zu", args.size());
        return false;
    }

    const std::string& module = *it++;

    const std::string& canonical_name = MakeCanonical(module);
    if (canonical_name.empty()) {
        return false;
    }
    this->module_blocklist_.emplace(canonical_name);

    return true;
}

void Modprobe::ParseCfg(const std::string& cfg,
                        std::function<bool(const std::vector<std::string>&)> f) {
    std::ifstream file(cfg);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        std::vector<std::string> args{std::istream_iterator<std::string>{iss},
                                      std::istream_iterator<std::string>{}};
        if (!args.empty()) {
            f(args);
        }
    }
}

bool Modprobe::IsBlocklisted(const std::string& module_name) {
    if (!blocklist_enabled) return false;

    auto canonical_name = MakeCanonical(module_name);
    auto dependencies = GetDependencies(canonical_name);
    for (auto dep = dependencies.begin(); dep != dependencies.end(); ++dep) {
        if (module_blocklist_.count(MakeCanonical(*dep))) return true;
    }

    return module_blocklist_.count(canonical_name) > 0;
}

// Loads kernel modules in parallel by resolving independent dependencies first,
// then iterating over remaining dependencies until all modules are loaded.
// Blocklisted modules are ignored. Soft dependencies are handled in InsmodWithDeps().
bool Modprobe::LoadModulesParallel(int num_threads) {
    bool ret = true;
    std::map<std::string, std::vector<std::string>> mod_with_deps;

    // Get dependencies for all modules
    for (const auto& module : module_load_) {
        // Skip blocklisted modules
        if (IsBlocklisted(module)) {
            LOGV("LMP: Blocklist error: Module %s is blocklisted", module.c_str());
            continue;
        }

        auto dependencies = GetDependencies(MakeCanonical(module));
        if (dependencies.empty()) {
            LOGE("LMP: Module %s not in dependency file", module.c_str());
            return false;
        }

        mod_with_deps[MakeCanonical(module)] = dependencies;
    }

    while (!mod_with_deps.empty()) {
        std::vector<std::thread> threads;
        std::vector<std::string> mods_path_to_load;
        std::mutex vector_lock;

        // Identify independent modules that can be loaded concurrently
        for (const auto& [mod_name, deps] : mod_with_deps) {
            if (deps.empty()) continue;

            auto last_dep = deps.back();
            auto canonical_dep = MakeCanonical(last_dep);

            // Ensure that hard dependencies are not blocklisted
            if (IsBlocklisted(canonical_dep)) {
                LOGV("LMP: Blocklist error: Module %s is blocklisted", canonical_dep.c_str());
                return false;
            }

            // Check for modules that require sequential loading
            std::string sequential_flag = "load_sequential=1";
            auto option_pos = module_options_[canonical_dep].find(sequential_flag);
            if (option_pos != std::string::npos) {
                // Remove sequential flag and load module sequentially
                module_options_[canonical_dep].erase(option_pos, sequential_flag.length());
                if (!LoadWithAliases(canonical_dep, true)) {
                    return false;
                }
            } else {
                // Add module to parallel load list if not already present
                if (std::find(mods_path_to_load.begin(), mods_path_to_load.end(), canonical_dep) ==
                    mods_path_to_load.end()) {
                    mods_path_to_load.emplace_back(canonical_dep);
                }
            }
        }

        // Parallel loading function
        auto thread_function = [&] {
            std::unique_lock<std::mutex> lock(vector_lock);
            while (!mods_path_to_load.empty()) {
                auto mod_to_load = std::move(mods_path_to_load.back());
                mods_path_to_load.pop_back();
                lock.unlock();

                bool load_success = LoadWithAliases(mod_to_load, true);

                lock.lock();
                if (!load_success) {
                    ret = false;
                }
            }
        };

        // Spawn worker threads for parallel module loading
        std::generate_n(std::back_inserter(threads), num_threads,
                        [&] { return std::thread(thread_function); });

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        if (!ret) return false;

        // Lock to update module dependencies after parallel load
        std::lock_guard<std::mutex> guard(module_loaded_lock_);

        // Remove successfully loaded modules from the dependency map
        for (const auto& loaded_module : module_loaded_) {
            mod_with_deps.erase(loaded_module);
        }

        // Remove loaded modules from dependencies of remaining modules
        for (const auto& loaded_path : module_loaded_paths_) {
            for (auto& [mod, deps] : mod_with_deps) {
                deps.erase(std::remove(deps.begin(), deps.end(), loaded_path), deps.end());
            }
        }
    }

    return ret;
}

bool Modprobe::LoadListedModules(bool strict) {
    bool ret = true;
    for (const auto& module : module_load_) {
        if (!LoadWithAliases(module, true)) {
            if (IsBlocklisted(module)) continue;
            ret = false;
            if (strict) break;
        }
    }
    return ret;
}

bool Modprobe::LoadWithAliases(const std::string& module_name, bool strict,
                               const std::string& parameters) {
    auto canonical_name = MakeCanonical(module_name);
    if (module_loaded_.count(canonical_name)) {
        return true;
    }

    std::set<std::string> modules_to_load = {canonical_name};
    bool module_loaded = false;

    for (const auto& [alias, aliased_module] : module_aliases_) {
        if (fnmatch(alias.c_str(), module_name.c_str(), 0) != 0) continue;
        LOGD("Found alias for '%s': '%s'", module_name.c_str(), aliased_module.c_str());
        if (module_loaded_.count(MakeCanonical(aliased_module))) continue;
        modules_to_load.emplace(aliased_module);
    }

    for (const auto& module : modules_to_load) {
        if (!ModuleExists(module)) continue;
        if (InsmodWithDeps(module, parameters)) module_loaded = true;
    }

    if (strict && !module_loaded) {
        LOGE("LoadWithAliases was unable to load %s", module_name.c_str());
        return false;
    }
    return true;
}

void Modprobe::AddOption(const std::string& module_name, const std::string& option_name,
                         const std::string& value) {
    auto canonical_name = MakeCanonical(module_name);
    auto options_iter = module_options_.find(canonical_name);
    auto option_str = option_name + "=" + value;
    if (options_iter != module_options_.end()) {
        options_iter->second = options_iter->second + " " + option_str;
    } else {
        module_options_.emplace(canonical_name, option_str);
    }
}

void Modprobe::ParseKernelCmdlineOptions(void) {
    std::string cmdline = GetKernelCmdline();
    std::string module_name = "";
    std::string option_name = "";
    std::string value = "";
    bool in_module = true;
    bool in_option = false;
    bool in_value = false;
    bool in_quotes = false;
    int start = 0;

    for (int i = 0; i < cmdline.size(); i++) {
        if (cmdline[i] == '"') {
            in_quotes = !in_quotes;
        }

        if (in_quotes) continue;

        if (cmdline[i] == ' ') {
            if (in_value) {
                value = cmdline.substr(start, i - start);
                if (!module_name.empty() && !option_name.empty()) {
                    AddOption(module_name, option_name, value);
                }
            }
            module_name = "";
            option_name = "";
            value = "";
            in_value = false;
            start = i + 1;
            in_module = true;
            continue;
        }

        if (cmdline[i] == '.') {
            if (in_module) {
                module_name = cmdline.substr(start, i - start);
                start = i + 1;
                in_module = false;
            }
            in_option = true;
            continue;
        }

        if (cmdline[i] == '=') {
            if (in_option) {
                option_name = cmdline.substr(start, i - start);
                start = i + 1;
                in_option = false;
            }
            in_value = true;
            continue;
        }
    }
    if (in_value && !in_quotes) {
        value = cmdline.substr(start, cmdline.size() - start);
        if (!module_name.empty() && !option_name.empty()) {
            AddOption(module_name, option_name, value);
        }
    }
}

Modprobe::Modprobe(const std::vector<std::string>& base_paths, const std::string load_file,
                   bool use_blocklist)
    : blocklist_enabled(use_blocklist) {
    using namespace std::placeholders;

    for (const auto& base_path : base_paths) {
        auto alias_callback = std::bind(&Modprobe::ParseAliasCallback, this, _1);
        ParseCfg(base_path + "/modules.alias", alias_callback);

        auto dep_callback = std::bind(&Modprobe::ParseDepCallback, this, base_path, _1);
        ParseCfg(base_path + "/modules.dep", dep_callback);

        auto softdep_callback = std::bind(&Modprobe::ParseSoftdepCallback, this, _1);
        ParseCfg(base_path + "/modules.softdep", softdep_callback);

        auto load_callback = std::bind(&Modprobe::ParseLoadCallback, this, _1);
        ParseCfg(base_path + "/" + load_file, load_callback);

        auto options_callback = std::bind(&Modprobe::ParseOptionsCallback, this, _1);
        ParseCfg(base_path + "/modules.options", options_callback);

        auto blocklist_callback = std::bind(&Modprobe::ParseBlocklistCallback, this, _1);
        ParseCfg(base_path + "/modules.blocklist", blocklist_callback);
    }

    ParseKernelCmdlineOptions();
}

std::vector<std::string> Modprobe::GetDependencies(const std::string& module) {
    auto it = module_deps_.find(module);
    if (it == module_deps_.end()) {
        return {};
    }
    return it->second;
}

bool Modprobe::InsmodWithDeps(const std::string& module_name, const std::string& parameters) {
    if (module_name.empty()) {
        LOGE("Need valid module name, given: %s", module_name.c_str());
        return false;
    }

    auto dependencies = GetDependencies(module_name);
    if (dependencies.empty()) {
        LOGE("Module %s not in dependency file", module_name.c_str());
        return false;
    }

    for (auto dep = dependencies.rbegin(); dep != dependencies.rend() - 1; ++dep) {
        LOGD("Loading hard dep for '%s': %s", module_name.c_str(), dep->c_str());
        if (!LoadWithAliases(*dep, true)) {
            return false;
        }
    }

    if (!Insmod(dependencies[0], parameters)) {
        return false;
    }

    return true;
}