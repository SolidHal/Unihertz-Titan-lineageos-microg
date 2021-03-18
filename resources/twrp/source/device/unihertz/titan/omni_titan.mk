PRODUCT_RELEASE_NAME := titan

$(call inherit-product, build/target/product/embedded.mk)

# Inherit from our custom product configuration
$(call inherit-product, vendor/omni/config/common.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := titan
PRODUCT_NAME := omni_titan
PRODUCT_BRAND := Unihertz
PRODUCT_MANUFACTURER := Unihertz
PRODUCT_MODEL := Titan


# HACK: Set vendor patch level
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.build.security_patch=2099-12-31 \
    ro.crypto.volume.filenames_mode=aes-256-cts \
    persist.sys.usb.config=mtp,adb \
    sys.usb.config=mtp,adb
