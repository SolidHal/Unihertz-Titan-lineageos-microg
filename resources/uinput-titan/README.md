include/uapi/linux/uinput.h
```
struct uinput_setup {
        struct input_id id;
        char name[UINPUT_MAX_NAME_SIZE];
        __u32 ff_effects_max;
};

/**
 * UI_DEV_SETUP - Set device parameters for setup
 *
 * This ioctl sets parameters for the input device to be created.  It
 * supersedes the old "struct uinput_user_dev" method, which wrote this data
 * via write(). To actually set the absolute axes UI_ABS_SETUP should be
 * used.
 *
 * The ioctl takes a "struct uinput_setup" object as argument. The fields of
 * this object are as follows:
 *              id: See the description of "struct input_id". This field is
 *                  copied unchanged into the new device.
 *            name: This is used unchanged as name for the new device.
 *  ff_effects_max: This limits the maximum numbers of force-feedback effects.
 *                  See below for a description of FF with uinput.
 *
 * This ioctl can be called multiple times and will overwrite previous values.
 * If this ioctl fails with -EINVAL, it is recommended to use the old
 * "uinput_user_dev" method via write() as a fallback, in case you run on an
 * old kernel that does not support this ioctl.
 *
 * This ioctl may fail with -EINVAL if it is not supported or if you passed
 * incorrect values, -ENOMEM if the kernel runs out of memory or -EFAULT if the
 * passed uinput_setup object cannot be read/written.
 * If this call fails, partial data may have already been applied to the
 * internal device.
 */
#define UI_DEV_SETUP _IOW(UINPUT_IOCTL_BASE, 3, struct uinput_setup)

struct uinput_abs_setup {
        __u16  code; /* axis code */
        /* __u16 filler; */
        struct input_absinfo absinfo;
};

/**
 * UI_ABS_SETUP - Set absolute axis information for the device to setup
 *
 * This ioctl sets one absolute axis information for the input device to be
 * created. It supersedes the old "struct uinput_user_dev" method, which wrote
 * part of this data and the content of UI_DEV_SETUP via write().
 *
 * The ioctl takes a "struct uinput_abs_setup" object as argument. The fields
 * of this object are as follows:
 *            code: The corresponding input code associated with this axis
 *                  (ABS_X, ABS_Y, etc...)
 *         absinfo: See "struct input_absinfo" for a description of this field.
 *                  This field is copied unchanged into the kernel for the
 *                  specified axis. If the axis is not enabled via
 *                  UI_SET_ABSBIT, this ioctl will enable it.
 *
 * This ioctl can be called multiple times and will overwrite previous values.
 * If this ioctl fails with -EINVAL, it is recommended to use the old
 * "uinput_user_dev" method via write() as a fallback, in case you run on an
 * old kernel that does not support this ioctl.
 *
 * This ioctl may fail with -EINVAL if it is not supported or if you passed
 * incorrect values, -ENOMEM if the kernel runs out of memory or -EFAULT if the
 * passed uinput_setup object cannot be read/written.
 * If this call fails, partial data may have already been applied to the
 * internal device.
 */
#define UI_ABS_SETUP _IOW(UINPUT_IOCTL_BASE, 4, struct uinput_abs_setup)
```

include/uapi/linux/input.h
```
struct input_id {
        __u16 bustype;
        __u16 vendor;
        __u16 product;
        __u16 version;
};

/**
 * struct input_absinfo - used by EVIOCGABS/EVIOCSABS ioctls
 * @value: latest reported value for the axis.
 * @minimum: specifies minimum value for the axis.
 * @maximum: specifies maximum value for the axis.
 * @fuzz: specifies fuzz value that is used to filter noise from
 *      the event stream.
 * @flat: values that are within this value will be discarded by
 *      joydev interface and reported as 0 instead.
 * @resolution: specifies resolution for the values reported for
 *      the axis.
 *
 * Note that input core does not clamp reported values to the
 * [minimum, maximum] limits, such task is left to userspace.
 *
 * The default resolution for main axes (ABS_X, ABS_Y, ABS_Z)
 * is reported in units per millimeter (units/mm), resolution
 * for rotational axes (ABS_RX, ABS_RY, ABS_RZ) is reported
 * in units per radian.
 * When INPUT_PROP_ACCELEROMETER is set the resolution changes.
 * The main axes (ABS_X, ABS_Y, ABS_Z) are then reported in
 * in units per g (units/g) and in units per degree per second
 * (units/deg/s) for rotational axes (ABS_RX, ABS_RY, ABS_RZ).
 */
struct input_absinfo {
        __s32 value;
        __s32 minimum;
        __s32 maximum;
        __s32 fuzz;
        __s32 flat;
        __s32 resolution;
};
```


include/uapi/linux/input.h
```
/**
 * struct input_absinfo - used by EVIOCGABS/EVIOCSABS ioctls
 * @value: latest reported value for the axis.
 * @minimum: specifies minimum value for the axis.
 * @maximum: specifies maximum value for the axis.
 * @fuzz: specifies fuzz value that is used to filter noise from
 *      the event stream.
 * @flat: values that are within this value will be discarded by
 *      joydev interface and reported as 0 instead.
 * @resolution: specifies resolution for the values reported for
 *      the axis.
 *
 * Note that input core does not clamp reported values to the
 * [minimum, maximum] limits, such task is left to userspace.
 *
 * The default resolution for main axes (ABS_X, ABS_Y, ABS_Z)
 * is reported in units per millimeter (units/mm), resolution
 * for rotational axes (ABS_RX, ABS_RY, ABS_RZ) is reported
 * in units per radian.
 * When INPUT_PROP_ACCELEROMETER is set the resolution changes.
 * The main axes (ABS_X, ABS_Y, ABS_Z) are then reported in
 * in units per g (units/g) and in units per degree per second
 * (units/deg/s) for rotational axes (ABS_RX, ABS_RY, ABS_RZ).
 */
struct input_absinfo {
        __s32 value;
        __s32 minimum;
        __s32 maximum;
        __s32 fuzz;
        __s32 flat;
        __s32 resolution;
};
```
