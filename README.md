## references
- titan tips: https://unihertz-titan.neocities.org
- discord with a ton information: https://discord.gg/PxnUeM8
- https://github.com/phhusson/unihertz_titan
- phhusson gsi: https://github.com/phhusson/treble_experimentations
- internal pictures: https://fccid.io/2AK6CTITAN/Internal-Photos/internal-Photos-4474419


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
 Portions of this process rely on portions of the stock rom. This was tested with android 10 ota from February 4/5 2021.
 If you have trouble, try flashing to that specific ota by following the instructions below in the `Recover bricked titan / restore to stock rom` section

1) Setup adb and fastboot on your computer
https://www.xda-developers.com/install-adb-windows-macos-linux/

2) enable adb debugging on your phone. Same guide above has instructions.

3) enable OEM bootloader unlock
- Find the OEM unlock toggle under the Developer Settings and flip it

4) get in the bootloader
```
adb reboot bootloader
```

5) once at the bootloader, finalize the unlock:
```
fastboot flashing unlock
```

press volume up to accept

6) Now wipe the device
```
fastboot erase system
fastboot erase cache
fastboot erase userdata
```

7) flash the gsi image to the system partition
- either download a prebuilt from the section above
- or follow the build section above
```
fastboot flash system <system.img>
```

#### If you don't microG, Magisk, Fdroid Privileged Extension, Aurora Services you can reboot now. Otherwise, Continue to `Install Magisk`


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


### aptX bluetooth
aptX won't work on this device without installing the magisk modules located in the `resources/bluetooth` directory

### Keyboard configuration

These map the keyboard properly, and remove the annoying cursor. Keyboard touchpad functionality is a bit trickier and is expanded on in a later section.

```
adb root
adb remount
adb shell mount -o remount,rw /
adb push resources/keyboard/Android10/system_usr_idc/* /system/usr/idc/
adb push resources/keyboard/Android10/system_usr_keychars/* /system/usr/keychars/
adb push resources/keyboard/Android10/system_usr_keylayout/* /system/usr/keylayout/
```

hide the software keyboard when using the hardware keyboard:
```
adb shell settings put secure show_ime_with_hard_keyboard 1
```

TODO: 
- binding for alt/ctrl/esc

### Touchpad configuration

since none of the suggested .idc modifications worked properly, I resurrected uinput-titan from https://github.com/phhusson/unihertz_titan

#### optional: build uinput-titan
I have included a prebuilt binary, but if it doesn't work for some reason heres how to build it.
get the android ndk from here: https://developer.android.com/ndk/downloads and extract it to ~/android-ndk
```
cd resources/uinput-titan
make
```

#### Install

```
adb root
adb remount
adb shell mount -o remount,rw /
adb push resources/keyboard/Android10_function/system_usr_idc/* /system/usr/idc/
adb push resources/keyboard/Android10_function/system_usr_keychars/* /system/usr/keychars/
adb push resources/keyboard/Android10_function/system_usr_keylayout/* /system/usr/keylayout/
adb push resources/uinput-titan/uinput-titan /system/bin/uinput-titan
adb push resources/uinput-titan/titan.rc /system/etc/init/
adb push resources/uinput-titan/titan-uinput.idc /system/usr/idc/
```

#### Functionalities

##### Added keyboard functionality
- single pressing alt or shift enters toggle mode
  - the next pressed key will be modified by alt or shift, and alt/shift will then be disabled
- quickly double pressing alt or shift enters lock mode
  - all keys pressed will be modified by alt/shift until alt/shift is pressed again
  - TODO: double tap on space mapped to tab
  - back mapped to ctrl key
  - recents/app_switch mapped to fn layer
    - HJKL or IJKL mapped to arrows
    - T/Y curly brace
    - TODO: make graphic that shows mapping
  
  
*note:TODO currently the alt+shift layer does not have any functionality so nothing will happen when keys are pressed with both modifiers active
 - 


##### By default the touchpad is in navigation mode
It is meant to act as an extension of the touch screen. 
It is designed to allow for gestures, scrolling vertically and horizontally, and swipes while preventing taps
When used with the "Gesture navigation", all navigation gestures can be performed on the touchpad
This allows for the "recents" and "back" keys to be remapped

##### TODO: pressing the red button on the left side of the device activates terminal mode

- swipes left and right are remapped to arrow keys
- 



### software keyboard 

ruKeyboard or anysoft

info on https://unihertz-titan.neocities.org


### Ui stuttering issues? 

likely not on this device, but good to note for the future:

try the following:
```
You have to paste the following code in your vendor build.prop (to prevent the lost of it during system flashes)

debug.sf.latch_unsignaled=1
```

## Recover bricked titan / restore to stock rom

1)  Flash the latest Android 9 official build. Android 10 images are only available at ota files so far. 
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
## WARNING: DO NOT SET AS FORMAT ALL + DOWNLOAD. YOU WILL WIPE YOUR MEID AND IMEI
- Click Download and wait. 
- Once it completes you may have to force power cycle the phone. If you get to a menu in Chinese, I found power cycling again to fix it but YMMV. 

2) Optional - Flash Official Android 10
Images can be found here: https://drive.google.com/drive/folders/1YgwDZoVZfNuKmdVuQIfRLA0hrYj0ndgS?fbclid=IwAR0hbuVoITZC7kBUHmN0rWS-kVWLu00e-GOQ4fkose-94BL77TZTtxdveWI

There are a few ways to apply ota updates, but I find using `adb sideload` the easiest. 

Make sure you have adb debugging enabled. Plug in your device and run:
```
adb reboot recovery
```

You should see a standard android recovery. If you just have the fallen over android, press power+volume up a few times.
Use the volume and power keys to select the adb sideload option and then run
```
adb sideload <android-10-ota.zip>
```




# Thanks to:
@ValdikSS - owner of https://unihertz-titan.neocities.org, patched boot.img for magisk, and twrp builds
          - much of this information comes from them, and I am just organizing it here
          
@phhusson - for the great base android GSI images
          - for the core of uinput-titan
