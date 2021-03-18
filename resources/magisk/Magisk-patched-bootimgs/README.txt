Unihertz Titan Android 10 Magisk-patched (v22.0) root-privileged bootimgs

These files are Magisk-patched boot.imgs from the following Android 10 firmware:
* 2021020416_g61v71c2k_dfl_eea-ota (for European devices)
* 2021020509_g61v71c2k_dfl_tee-ota (for non-European devices)

NOTE: do NOT try to install these images if newer firmware version is available. If you're reading this in the future, where 2021020416/2021020509 aren't the latest ones, do not use the images!

boot_*.img files are ORIGINAL ones (left for convenience), magisk_patched_*.img are PATCHED.

During Android 10 upgrade, Unihertz switched from custom old dm-verity implementation to AVB 2.0, but without dedicated vbmeta partition. Instead, vbmeta is expected to be attached to boot and recovery partitions, and the empty one leads to boot failure.

Magisk v22.0 wipes attached vbmeta data, that's why currently (as of 24 Feb 2021) it can't be used to patch boot.img directly.
You can track the issue on Magisk bug tracker: https://github.com/topjohnwu/Magisk/issues/3727
WARNING: Do not update Magisk using Magisk Manager, you'll get bootloop!


# Installation

NOTE: these files are compatible only with Android 10. Make sure to install Android 10 image on Titan first.

Method 1:

1. Determine your version: EEA or TEE. Go to "About Phone" - "Build number". If it begins with "Titan_", that's TEE. If it's "Titan_EEA" - that's EEA. Use proper image corresponding to your firmware region.
2. Allow OEM unlocking in Android developer settings (if you don't know what it is, search on the internet)
3. Boot to fastboot: power off the device, press VOL UP + POWER, you'll get into recovery, select "boot into bootloader". If you see "no command" screen, press VOL UP + POWER simultaneously multiple times, you'll eventually see the menu.
4. (skip this step if you already have unlocked bootloader) On the PC: fastboot flashing unlock
5. On the PC: fastboot flash boot magisk_patched_fixed_boot_Titan_2021020416_g61v71c2k_dfl_eea-ota.img (for example)
6. fastboot reboot

Method 2:

Use SP Flash Tool with stock Titan firmware., but select only "boot" partition to update, using magisk_patched_fixed_boot_Titan_2021020416_g61v71c2k_dfl_eea-ota.img (for example) as the boot file.
Bootloader should be unlocked (see above).


WARNING: Do not update Magisk using Magisk Manager, you'll get bootloop!
As of 24 Feb 2021 and Magisk v22.0, Magisk Manager can't properly patch the image.
