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
const std::string PROP_RO_HARDWARE_GPU = "ro.hardware.gpu";
const std::string PROP_RO_HARDWARE_AUDIO = "ro.hardware.audio";
const std::string PROP_RO_HARDWARE_CAMERA = "ro.hardware.camera";
const std::string PROP_RO_HARDWARE_SENSOR = "ro.hardware.sensor";
const std::string PROP_RO_HARDWARE_KEYSTORE = "ro.hardware.keystore";
const std::string PROP_RO_HARDWARE_NFC = "ro.hardware.nfc";
const std::string PROP_RO_HARDWARE_WIFI = "ro.hardware.wifi";
const std::string PROP_RO_HARDWARE_BLUETOOTH = "ro.hardware.bluetooth";
const std::string PROP_RO_HARDWARE_USB = "ro.hardware.usb";
const std::string PROP_RO_HARDWARE_ETHER = "ro.hardware.ether";
const std::string PROP_RO_HARDWARE_GPS = "ro.hardware.gps";
const std::string PROP_RO_HARDWARE_RADIO = "ro.hardware.radio";
const std::string PROP_RO_HARDWARE_TELEPHONY = "ro.hardware.telephony";
const std::string PROP_RO_HARDWARE_POWER = "ro.hardware.power";
const std::string PROP_RO_HARDWARE_LIGHT = "ro.hardware.light";
const std::string PROP_RO_HARDWARE_VIBRATOR = "ro.hardware.vibrator";
const std::string PROP_RO_HARDWARE_INPUT = "ro.hardware.input";
const std::string PROP_RO_HARDWARE_DISPLAY = "ro.hardware.display";
const std::string PROP_RO_HARDWARE_TOUCHSCREEN = "ro.hardware.touchscreen";
const std::string PROP_RO_HARDWARE_KEYBOARD = "ro.hardware.keyboard";
const std::string PROP_RO_HARDWARE_MOUSE = "ro.hardware.mouse";
const std::string PROP_RO_HARDWARE_TRACKBALL = "ro.hardware.trackball";
const std::string PROP_RO_HARDWARE_JOYSTICK = "ro.hardware.joystick";
const std::string PROP_RO_HARDWARE_HDMI = "ro.hardware.hdmi";
const std::string PROP_RO_HARDWARE_SDCARD = "ro.hardware.sdcard";
const std::string PROP_RO_HARDWARE_EMMC = "ro.hardware.emmc";
const std::string PROP_RO_HARDWARE_UFS = "ro.hardware.ufs";
const std::string PROP_RO_HARDWARE_FLASH = "ro.hardware.flash";
const std::string PROP_RO_HARDWARE_BATTERY = "ro.hardware.battery";
const std::string PROP_RO_HARDWARE_THERMAL = "ro.hardware.thermal";
const std::string PROP_RO_HARDWARE_FINGERPRINT = "ro.hardware.fingerprint";
const std::string PROP_RO_HARDWARE_IR = "ro.hardware.ir";
const std::string PROP_RO_HARDWARE_FM = "ro.hardware.fm";
const std::string PROP_RO_HARDWARE_TV = "ro.hardware.tv";
const std::string PROP_RO_HARDWARE_CAMERA_FLASH = "ro.hardware.camera.flash";
const std::string PROP_RO_HARDWARE_CAMERA_FRONT = "ro.hardware.camera.front";
const std::string PROP_RO_HARDWARE_CAMERA_REAR = "ro.hardware.camera.rear";
const std::string PROP_RO_HARDWARE_MICROPHONE = "ro.hardware.microphone";
const std::string PROP_RO_HARDWARE_SPEAKER = "ro.hardware.speaker";
const std::string PROP_RO_HARDWARE_HEADSET = "ro.hardware.headset";
const std::string PROP_RO_HARDWARE_HEADPHONE = "ro.hardware.headphone";
const std::string PROP_RO_HARDWARE_LINEOUT = "ro.hardware.lineout";
const std::string PROP_RO_HARDWARE_AUX = "ro.hardware.aux";
const std::string PROP_RO_HARDWARE_USB_AUDIO = "ro.hardware.usb.audio";
const std::string PROP_RO_HARDWARE_HDMI_AUDIO = "ro.hardware.hdmi.audio";
const std::string PROP_RO_HARDWARE_BT_AUDIO = "ro.hardware.bt.audio";
const std::string PROP_RO_HARDWARE_WIFI_DIRECT = "ro.hardware.wifi.direct";
const std::string PROP_RO_HARDWARE_WIFI_DISPLAY = "ro.hardware.wifi.display";
const std::string PROP_RO_HARDWARE_WIFI_P2P = "ro.hardware.wifi.p2p";
const std::string PROP_RO_HARDWARE_WIFI_AP = "ro.hardware.wifi.ap";
const std::string PROP_RO_HARDWARE_WIFI_STA = "ro.hardware.wifi.sta";
const std::string PROP_RO_HARDWARE_WIFI_HOTSPOT = "ro.hardware.wifi.hotspot";
const std::string PROP_RO_HARDWARE_WIFI_PASSPOINT = "ro.hardware.wifi.passpoint";
const std::string PROP_RO_HARDWARE_WIFI_TDLS = "ro.hardware.wifi.tdls";
const std::string PROP_RO_HARDWARE_WIFI_RTT = "ro.hardware.wifi.rtt";
const std::string PROP_RO_HARDWARE_WIFI_AWARE = "ro.hardware.wifi.aware";
const std::string PROP_RO_HARDWARE_WIFI_MIRACAST = "ro.hardware.wifi.miracast";
const std::string PROP_RO_HARDWARE_WIFI_SCAN = "ro.hardware.wifi.scan";
const std::string PROP_RO_HARDWARE_WIFI_OFFLOAD = "ro.hardware.wifi.offload";
const std::string PROP_RO_HARDWARE_WIFI_HAL = "ro.hardware.wifi.hal";
const std::string PROP_RO_HARDWARE_WIFI_LEGACY_HAL = "ro.hardware.wifi.legacy_hal";
const std::string PROP_RO_HARDWARE_WIFI_VENDOR_HAL = "ro.hardware.wifi.vendor_hal";
const std::string PROP_RO_HARDWARE_WIFI_EXTENDED_HAL = "ro.hardware.wifi.extended_hal";

#endif // INIT_CONSTANTS_H