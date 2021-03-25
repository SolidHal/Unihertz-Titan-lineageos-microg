
# get the ndk here:https://developer.android.com/ndk/downloads

ANDROID_NDK ?= ~/android-ndk/
LOCAL_LDLIBS := -llog

uinput-titan: uinput-titan.c
	$(ANDROID_NDK)toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang uinput-titan.c -o uinput-titan -Wall -Wextra -Werror -Wl,-llog
