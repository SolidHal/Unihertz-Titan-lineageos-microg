## references
titan tips: https://unihertz-titan.neocities.org
discord with a ton information: https://discord.gg/PxnUeM8
https://github.com/phhusson/unihertz_titan
phhusson gsi: https://github.com/phhusson/treble_experimentations

internal pictures: https://fccid.io/2AK6CTITAN/Internal-Photos/internal-Photos-4474419


## Prebuilt images

Andy Yan: https://sourceforge.net/projects/andyyan-gsi/files/lineage-17.x/
- Need the `arm64_bvS` image

## Build

https://github.com/AndyCGYan/treble_build_los/tree/lineage-17.1

## Unihertz titan key combos:
recovery : power cycle and hold volume up. If you just get the broken droid, press or hold volume up some more, or possibly power + volume up.
- note: unfortunately if you end up bootlooping for some reason, recovery is likely also inaccessible
- from here you can also `reboot to bootloader`
- if we ever get kernel source, we could also `adb sideload` an image here. Unihertz seems to be uninterested in following the GPL...
- Finally, this key combo is pretty finicky

## Install

### IMPORTANT!!
1)  Flash the latest Android 9 official build
the Android 10 update from upstream seems to break compatibility with GSI images, not sure why.
- need to use SPFlashTool to flash
  - https://spflashtool.com/
  - ubuntu linux needs the following pre-requisites:
  - TODO: old libpng modified to work on newer ubuntu versions. Can be found on this repo
  - further setup instructions: https://forum.xda-developers.com/t/tutorial-how-to-setup-sp_flash_tool_linux-mtk-mediatek-soc.3160802/
  
- get the official image:
2020080711_g61v71c2k_dfl_tee.zip
from here: https://drive.google.com/drive/folders/1fpO65z2_r9zT8UImuV8-5JCRBy33yZcV


- extract the image.
- Open SPFlashTool, plug in your device, SPFlashTool should recognize it and put the cpu name in the upper left.
- Go to the Download tab
- leave the Download-Agent as default
- For Scatter-loading File choose the `MT6771_Android_scatter.txt` in your extracted image
- leave the setting as `Download Only`
- Click Download and wait. 
- Once it completes you may have to force power cycle the phone. If you get to a menu in Chinese, I found power cycling again to fix it but YMMV. 

2) Setup adb and fastboot on your computer
https://www.xda-developers.com/install-adb-windows-macos-linux/

3) enable adb debugging on your phone. Same guide above has instructions.

4) enable OEM bootloader unlock
- Find the OEM unlock toggle under the Developer Settings and flip it

5) get in the bootloader
```
adb reboot bootloader
```

6) once at the bootloader, finalize the unlock:
```
fastboot flashing unlock
```

7) Now wipe the device
```
fastboot erase system
fastboot erase cache
fastboot erase userdata
```

8) flash the gsi image to the system partition
- either download a prebuilt from the section above
- or follow the build section above
```
fastboot flash system <system.img>
```

#### If you don't want anything like TWRP, microG, Magisk, Fdroid Privileged Extension, Aurora Services you can reboot now. Otherwise, Continue to `Install TWRP`


# TODO: since twrp isn't really working, do we need it?

### Install TWRP
Image located in `resources/twrp/binary` directory came from https://unihertz-titan.neocities.org/#Unihertz%20Titan%20TWRP%20recovery%20%26%20rooting

```
fastboot flash recovery recovery.img
## Start pressing and holding VOLUME UP, or you will return to stock recovery
fastboot reboot
```


### Install Magisk

On official Android 10 image, Magisk 22.0 (as of 24 Feb 2021) can't be used to patch boot.img due to bug. Use the following patched boot.img files with Magisk 22.0 pre-installed:

#### These work only for 4/5 Feb 2021 firmware!
#### WARNING: Do not update Magisk using Magisk Manager, you'll get bootloop!

the patched boot images can be found in `resources/magisk/Magisk-patched-bootimgs`

* 2021020416_g61v71c2k_dfl_eea-ota (for European devices)
* 2021020509_g61v71c2k_dfl_tee-ota (for non-European devices)

tee:
```
fastboot flash boot magisk_patched_fixed_boot_2021020509_g61v71c2k_dfl_tee-ota.img
```

eea:
```
fastboot flash boot magisk_patched_fixed_boot_Titan_2021020416_g61v71c2k_dfl_eea-ota.img
```

## TODO add instructions to build your own patched boot.img



### Install MicroG
nanolx has nice zips you can flash
https://downloads.nanolx.org/NanoDroid/Stable/
download the microG one

or use this magisk module?
https://github.com/nift4/microg_installer_revived
TODO: build into rom?

### Install Fdroid Privileged Extension
https://downloads.nanolx.org/NanoDroid/Stable/
- TODO: build into rom?
- use magisk module?
https://github.com/Magisk-Modules-Repo/Fdroid-Priv

### Install Aurora services
https://gitlab.com/AuroraOSS/AuroraServices
- TODO: build into rom?
- use magisk modules

### Install basic packages
https://github.com/SolidHal/android_prebuilts_solidhal
- TODO: build into rom?


## TODO
 - add function to rom, or create magisk module to map button for switching between trackpad/navigation setting
 - get reasonable keyboard setup
   - bind alt so we can use numbers
   - double tap alt for ctrl? get esc?

## Recover bricked titan / restore to stock rom

1)  Flash the latest Android 9 official build
- need to use SPFlashTool to flash
  - https://spflashtool.com/
  - ubuntu linux needs the following pre-requisites:
  - TODO: old libpng modified to work on newer ubuntu versions. Can be found on this repo
  - further setup instructions: https://forum.xda-developers.com/t/tutorial-how-to-setup-sp_flash_tool_linux-mtk-mediatek-soc.3160802/
  
- get the official image:
2020080711_g61v71c2k_dfl_tee.zip
from here: https://drive.google.com/drive/folders/1fpO65z2_r9zT8UImuV8-5JCRBy33yZcV


- extract the image.
- Open SPFlashTool, plug in your device, SPFlashTool should recognize it and put the cpu name in the upper left.
- Go to the Download tab
- leave the Download-Agent as default
- For Scatter-loading File choose the `MT6771_Android_scatter.txt` in your extracted image
- leave the setting as `Download Only`
## WARNING: DO NOT SET AS FORMAT ALL + DOWNLOAD. YOU WILL HAVE TO CONTACT SUPPORT TO GET YOUR IMEI/MEID/GOOGLE ATTESTATION SET AGAIN. 
- Click Download and wait. 
- Once it completes you may have to force power cycle the phone. If you get to a menu in Chinese, I found power cycling again to fix it but YMMV. 

2) Optional - Flash Official Android 10
Images can be found here: https://drive.google.com/drive/folders/1YgwDZoVZfNuKmdVuQIfRLA0hrYj0ndgS?fbclid=IwAR0hbuVoITZC7kBUHmN0rWS-kVWLu00e-GOQ4fkose-94BL77TZTtxdveWI

### TODO: switch between mouse and navigation mode for trackpad quickly?



### Fully functional ctrl key
```
Adding a fully functional CTRL key. (Root only) 

Basic steps to change the BACK key (or RECENTS key) to the CTRL_LEFT key (only if rooted):

key 580 = Recents key
key 158 =  Back key

1º Install Total Commander app and open it. 
2º Go to /system/usr/keylayout/Generic.kl, click on it and edit it.
3º Choose:
       - If you want to replace BACK key: search "key 158 BACK" and replace for "key 158 CTRL_LEFT".
       - If you want to replace RECENTS key: search "KEY 580 APP_SWITCH" and replace for "key 580 CTRL_LEFT".
4º Save the file.
5º Reboot the phone.

To remap holding back or recents to regain functionality use a button remapper of choice. 
Thanks to @diejuse on the discord
```



### Ui stuttering issues? 

likely not on this device, but good to note for the future:

try the following:
```
You have to paste the following code in your vendor build.prop (to prevent the lost of it during system flashes)

debug.sf.latch_unsignaled=1
```


# Thanks to:
@ValdikSS - owner of https://unihertz-titan.neocities.org, patched boot.img for magisk, and twrp builds
          - much of this information comes from them, and I am just organizing it here
