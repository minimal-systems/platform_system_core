// init_constants.h
// This header defines constants for Android system properties.

#ifndef INIT_CONSTANTS_H
#define INIT_CONSTANTS_H

#include <string>

// System property keys
const std::string PROP_RO_HARDWARE = "ro.hardware";
const std::string PROP_RO_BOOTLOADER = "ro.bootloader";
const std::string PROP_RO_PRODUCT_DEVICE = "ro.product.device";
const std::string PROP_RO_PRODUCT_MANUFACTURER = "ro.product.manufacturer";
const std::string PROP_RO_BUILD_DESCRIPTION = "ro.build.description";
const std::string PROP_RO_BUILD_VERSION_RELEASE = "ro.build.version.release";
const std::string PROP_RO_BUILD_VERSION_SDK = "ro.build.version.sdk";
const std::string PROP_RO_PRODUCT_MODEL = "ro.product.model";
const std::string PROP_RO_PRODUCT_BRAND = "ro.product.brand";
const std::string PROP_RO_SERIALNO = "ro.serialno";
const std::string PROP_RO_BOOT_IMAGE_BUILD_DATE = "ro.bootimage.build.date";
const std::string PROP_RO_BOOT_IMAGE_BUILD_DATE_UTC = "ro.bootimage.build.date.utc";
const std::string PROP_RO_BOOT_IMAGE_BUILD_FINGERPRINT = "ro.bootimage.build.fingerprint";
const std::string PROP_RO_DEBUGGABLE = "ro.debuggable";
const std::string PROP_RO_SECURE = "ro.secure";
const std::string PROP_RO_KERNEL_QEMU = "ro.kernel.qemu";
const std::string PROP_RO_SECURE_BOOT = "ro.boot.secureboot";
const std::string PROP_RO_SECURE_BOOT_STATE = "ro.boot.secureboot.state";
// gki specific
const std::string PROP_RO_GKI_BUILD = "ro.gki.build";
const std::string PROP_RO_GKI_BUILD_VERSION = "ro.gki.kernel_version";

#endif  // INIT_CONSTANTS_H