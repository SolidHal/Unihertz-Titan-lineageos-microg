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

6) Now wipe the device:
```
fastboot erase system
fastboot erase cache
fastboot erase userdata
```
#### Sometimes fastboot fails to detect some of your device's partitions. If you encounter this issue at this or a later step, try rebooting fastboot in "fastbootD" mode:
```
fastboot reboot fastboot
```

7) flash the gsi image to the system partition
- either download a prebuilt from the section above
- or follow the build section above
```
fastboot flash system <system.img>
```
#### If you don't microG, Magisk, Fdroid Privileged Extension, Aurora Services you can reboot now. Otherwise, Continue to `Install Magisk`


### Install Magisk

TODO: bug was fixed, update instructions. eg how to get boot.img and patch it
https://github.com/topjohnwu/Magisk/issues/3727

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

##### TODO add instructions to build your own patched boot.img


### Install MicroG
or use this magisk module
https://github.com/nift4/microg_installer_revived
TODO: build into rom?

### Install Fdroid Privileged Extension
- use magisk module?
https://github.com/Magisk-Modules-Repo/Fdroid-Priv
- TODO: build into rom?

### Install Aurora services
https://gitlab.com/AuroraOSS/AuroraServices
- use magisk modules
- TODO: build into rom?

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
adb reboot
```

if toggle/lock shift or alt doesn't work do another reboot. There seems to be a bug in the Android InputReader class or the kernel that causes this. Only input devices with `type ALPHA` keychars get this feature, so I'm guessing its some issue with that. 

#### Removal
```
adb root
adb remount
adb shell mount -o remount,rw /
adb shell rm  /system/bin/uinput-titan
adb shell rm  /system/etc/init/titan.rc
adb shell rm  /system/usr/idc/titan-uinput.idc
adb push resources/keyboard/Android10/system_usr_idc/* /system/usr/idc/
adb push resources/keyboard/Android10/system_usr_keychars/* /system/usr/keychars/
adb push resources/keyboard/Android10/system_usr_keylayout/* /system/usr/keylayout/
adb reboot
```

### Further keyboard configuration
I use Key Mapper from Fdroid to remap the red side key to CTRL. Doing this with keylayout files doesn't work since android doesn't let the two keyboards interact. Remapping this with uinput-titan also doesn't work for a number of reasons.

### Setting Configuration

This sets the display timeout to 5 minutes, scales down the display, and changes to gesture navigation, and hides the navigation hint
```
adb root
adb shell settings put system screen_off_timeout 600000
adb shell settings put system display_density_forced 320
adb shell settings put system system navigation_mode 2
adb shell settings put system --lineage system navigation_bar_hint 0
```


#### Functionalities

##### Added keyboard functionality
  - shift+alt to access programming layer.
  - short pressing the App Switch key also toggles on the programming layer. Long pressing activates the app switcher as usual.

```
| ; | | | [ | ] | { | } |   |   |   |     |
| & | ` | ^ | = | % | ← | ↓ | ↑ | → | del |
|   | < | > |   |   |   |   |   |   |     |
```
  
#### Design / TODO
TODO: add functionality: double tap on trackpad to enter cursor scrolling mode
- TODO: swipes left and right are remapped to arrow keys, or use volume arrow keys? if use volume arrows, get adb command.
- TODO: double tap on space mapped to tab


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

### useful adb debugging commands
```
adb logcat | grep "UINPUT"
```
```
adb shell getevent -i
```
```
adb shell getevent -v
```

## Recover bricked titan / restore to stock rom

1)  Flash the latest Android 9 official build. Android 10 images are only available at ota files so far. 
- need to use SPFlashTool to flash
  - https://spflashtool.com/
    - ubuntu linux needs the following pre-requisites:
      - TODO: old libpng modified to work on newer ubuntu versions. Can be found on this repo
    - On Arch and Arch-based Linux distributions you'll need to install `libpng12` from the community repository.
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


# kernel research

### keyboard
Looking into the driver of the titan keyboard, logs seem to call it aw9523_key
I can find two different references to a driver by this name, both in odd places. 
the first is a random git gist: https://gist.github.com/tablatronix/318368fd0f66958f413f0ac24a2a50e9 which implies it is from 2017
the second is from the gemian project: https://github.com/gemian/cosmo-linux-kernel-4.4/tree/master/drivers/misc/mediatek/aw9523 which seems to have originated in 2016. Both seem to be originally authored by AWINIC Technology CO who are the makers of the aw9523 gpio expander. 

Based on the log formatting:

aw9523_key_work: cnt = 30 key_report: p0-row=4, p1-col=2, key_code=158, key_val=1 keymap5 --- 2


it seems the titan is running a modified version of the one from the random gist. Unfortunately there isn't any sign of an aw9523 driver in the upstream android driver, or upstream linux.

That said, early this year someone tried to get a basic driver for the chip into upstream linux: https://www.spinics.net/lists/devicetree/msg402971.html

This gives some hope that we could get the keyboard working on a build of the upstream android kernel.
the fxtec pro1 uses the same chip for its keyboard, and the sailfish os folks use this driver that could also be modified to work https://github.com/sailfish-on-fxtecpro1/kernel-fxtec-pro1/blob/hybris-msm-aosp-9.0/drivers/input/keyboard/aw9523b.c


# Thanks to:
@ValdikSS - owner of https://unihertz-titan.neocities.org, patched boot.img for magisk, and twrp builds
          - much of this information comes from them, and I am just organizing it here
          
@phhusson - for the great base android GSI images
          - for the core of uinput-titan
