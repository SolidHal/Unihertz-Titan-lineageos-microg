#define _GNU_SOURCE
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <android/log.h>
#include <sys/time.h>

#define  LOG_TAG    "UINPUT-TITAN"

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

//now() is in total us mod 10^15
uint64_t now() {
    uint64_t t;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_usec;
    t += (tv.tv_sec % (1000*1000*1000)) * 1000*1000LL;
    return t;
}

static uint64_t lastKbdTimestamp;



static void insertEvent(int fd, int type, int code, int value) {
    struct input_event out_e;
    memset(&out_e, 0, sizeof(out_e));
    gettimeofday(&out_e.time, NULL);
    out_e.type = type;
    out_e.code = code;
    out_e.value = value;
    write(fd, &out_e, sizeof(out_e));
}

static int uinput_init() {
    int fd = open("/dev/uinput", O_RDWR);

    struct uinput_setup setup = {
        .id = {
               .bustype = BUS_VIRTUAL,
               .vendor = 0xdead,
               .product = 0xbeef,
               .version = 3,
               },
        .name = "titan-uinput",
        .ff_effects_max = 0,
    };
    ioctl(fd, UI_DEV_SETUP, setup);

    struct uinput_abs_setup abs_setup_x = {
        .code = ABS_X,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 1440,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 1440,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_x);

    //while the touchpad only goes to 720 in the y, we want to map directly to the display so we use 1440 */
    struct uinput_abs_setup abs_setup_y = {
        .code = ABS_Y,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 720,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 720,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_y);

    struct uinput_abs_setup abs_setup_pressure = {
        .code = ABS_PRESSURE,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 255,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_pressure);

    struct uinput_abs_setup abs_setup_touch_major = {
        .code = ABS_MT_TOUCH_MAJOR,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 100,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_touch_major);

    struct uinput_abs_setup abs_setup_touch_minor = {
        .code = ABS_MT_TOUCH_MINOR,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 100,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_touch_minor);

    struct uinput_abs_setup abs_setup_position_x = {
        .code = ABS_MT_POSITION_X,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 1440,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_position_x);

    //while the touchpad only goes to 720 in the y, we want to map directly to the display so we use 1440 */
    struct uinput_abs_setup abs_setup_position_y = {
        .code = ABS_MT_POSITION_Y,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 720,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_position_y);

    struct uinput_abs_setup abs_setup_tracking_id = {
        .code = ABS_MT_TRACKING_ID,
        .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 10,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 0,
                    },
    };
    ioctl(fd, UI_ABS_SETUP, abs_setup_tracking_id);


    // linux event codes defined here: https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h
    // more accurate android event codes defined here: https://android.googlesource.com/kernel/common/+/android-4.14-p/include/uapi/linux/input-event-codes.h
    //general events
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);

    // keyboard events
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_TAB);
    ioctl(fd, UI_SET_KEYBIT, KEY_PAGEUP);
    ioctl(fd, UI_SET_KEYBIT, KEY_PAGEDOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_FN);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHTALT);

    // lets us behave as a touchscreen. Inputs are directly mapped onto display
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    // touch events
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);

    const char phys[] = "this/is/a/virtual/device/for/scrolling";
    ioctl(fd, UI_SET_PHYS, phys);
    ioctl(fd, UI_DEV_CREATE, NULL);
    return fd;
}


static int open_ev(const char *lookupName) {
    char *path = NULL;
    for(int i=0; i<64;i++) {
        asprintf(&path, "/dev/input/event%d", i);
        int fd = open(path, O_RDWR);
        if(fd < 0) continue;
        char name[128];
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if(strcmp(name, lookupName) == 0) {
            LOGI("open_ev name = %s and lookupName = %s\n",name, lookupName);
            return fd;
        }

        close(fd);
    }
    free(path);
    return -1;
}

static int original_input_init() {
    int fd = open_ev("mtk-pad");
    if(fd<0) return fd;
    ioctl(fd, EVIOCGRAB, 1);
    return fd;
}


static void ev_parse_rel(struct input_event e) {
    const char *relName = "Unknown";
    switch(e.code) {
        default:
            fprintf(stderr, "Unknown rel %d\n", e.code);
            break;
    }
    printf("Got rel event %s: %d\n", relName, e.value);
}

static void ev_parse_syn(struct input_event e) {
    const char *synName = "Unknown";
    switch(e.code) {
        case SYN_REPORT:
            synName = "Simple report\n";
            break;
        case SYN_MT_REPORT:
            synName = "Multi-touch report";
            break;
        default:
            fprintf(stderr, "Unknown syn %d\n", e.code);
            break;
    }
    printf("Got syn event %s: %d\n", synName, e.value);
}

static void ev_parse_key(struct input_event e) {
    const char *keyName = "Unknown";
    switch(e.code) {
        case BTN_TOOL_FINGER:
            keyName = "tool finger";
            break;
        case BTN_TOUCH:
            keyName = "touch";
            break;
        default:
            fprintf(stderr, "Unknown key %d\n", e.code);
            break;
    }
    printf("Got key event %s: %d\n", keyName, e.value);
}

static void ev_parse_abs(struct input_event e) {
    const char *absName = "Unknown";
    switch(e.code) {
        case ABS_MT_POSITION_X:
            absName = "MT X";
            break;
        case ABS_MT_POSITION_Y:
            absName = "MT Y";
            break;
        case ABS_MT_TOUCH_MAJOR:
            absName = "Major touch axis";
            break;
        case ABS_MT_TOUCH_MINOR:
            absName = "Minor touch axis";
            break;
        case ABS_X:
            absName = "X";
            break;
        case ABS_Y:
            absName = "Y";
            break;
        case ABS_MT_TRACKING_ID:
            absName = "MT tracking id";
            break;
        default:
            fprintf(stderr, "Unknown abs %x\n", e.code);
            break;
    }
    printf("Got abs event %s: %d\n", absName, e.value);
}

int isInRect(int x, int y, int top, int bottom, int left, int right) {
    return
        (x > left && x < right && y > top && y < bottom);
}


int injectKeyDown(int ufd, int key){
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-INJECT", "injecting down key = %d\n", key);
    insertEvent(ufd, EV_KEY, key, 1);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
    return 0;
}

int injectKeyUp(int ufd, int key){
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-INJECT", "injecting up key = %d\n", key);
    insertEvent(ufd, EV_KEY, key, 0);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
    return 0;
}

int injectKey(int ufd, int key) {
    injectKeyDown(ufd, key);
    injectKeyUp(ufd, key);
    return 0;
}

int injectAbsEvent(int ufd, int x, int y, bool first){
    insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
    if(first){
        insertEvent(ufd, EV_KEY, BTN_TOUCH, 1 );
    }
    insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, x );
    insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, y );
    insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
    insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );
    insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
    return 0;
}

int injectAbsFinal(ufd){
    insertEvent(ufd, EV_KEY, BTN_TOUCH, 0 );
    insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, -1 );
    insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
    return 0;
}

// 0x0             X axis
// |----------------------------------|
// |                                  |
// |                                  |
// |                                  |
// |                                  |
// |                                  |
// |                                  |
// |                                  | Y axis
// |                                  |
// |                                  |
// |                                  |
// |                                  |
// |                                  |
// |----------------------------------| 1440x1440
// display resolution is 1440x1400
// keyboard touchpad resolution is 1440x720
// using system input swipe is much much too slow :/
// currently, this is only used for vertical swipes, but it has the capabilities to be used for horizontal swipes as well
int injectSwipe(int ufd, int xstart, int ystart, int xend, int yend){

    int x = 720;
    int y = 720;
    int swipe_size = 60;
    int swipe_increment = 20;
    bool vertical = false;
    bool positive = false; // positive is down, !positive is up
    if ( abs(xstart - xend ) > abs(ystart - yend)){
        vertical = false;
        if( ( xstart - xend ) > 0){
            positive = true;
        }
        else{
            positive = false;
        }
    }
    else{
        vertical = true;
        if( ( ystart - yend ) > 0){
            positive = true;
        }
        else{
            positive = false;
        }
    }

    LOGI("SENDING SWIPE\n");
    injectAbsEvent(ufd, x, y, true);
    if(vertical && positive){
        for(int j = y+swipe_increment; j <= y+swipe_size; j += swipe_increment){
            injectAbsEvent(ufd, x, j, false);
            usleep(500);
        }
    }
    if(vertical && !positive){
        for(int j = y-swipe_increment; j >= y-swipe_size; j -= swipe_increment){
            injectAbsEvent(ufd, x, j, false);
            usleep(500);
        }
    }
    if(!vertical && positive){
        for(int i = x+swipe_increment; i >= x+swipe_size; i += swipe_increment){
            injectAbsEvent(ufd, i, y, false);
            usleep(500);
        }
    }

    if(!vertical && !positive){
        for(int i = x-swipe_increment; i >= x-swipe_size; i -= swipe_increment){
            injectAbsEvent(ufd, i, y, false);
            usleep(500);
        }
    }

    injectAbsFinal(ufd);
    LOGI("SENT SWIPE\n");

    return 0;
}


struct input_event input_event_buffer[2048];
static int buffer_index = 0;
static int touched = 0;
static int sent_events = 0;
static int first_x = -1;
static int first_y = -1;
static int latest_x = -1;
static int latest_y = -1;
// since we are mapping a 720 resolution touchpad onto a 1440 resolution screen, we have to multiply the y value to
// get decent scrolling behaviour
static float y_multiplier = 1;
// TODO: pick a proper multiplier. best so far is 1.7 for y
static float x_multiplier = 1;


static void buffer(struct input_event e){
    input_event_buffer[buffer_index] = e;
    buffer_index++;
    return;
}

static void reset(){
    LOGI("reset\n");
    buffer_index = 0;
    touched = 0;
    sent_events = 0;
    first_x = -1;
    first_y = -1;
    latest_x = -1;
    latest_y = -1;
    return;
}

static void replay_buffer(int ufd, int correct_x){
    if (correct_x){
        for(int i = 0; i < buffer_index; i++){
            if(input_event_buffer[i].type == EV_ABS && input_event_buffer[i].code == ABS_MT_POSITION_X){
                input_event_buffer[i].value = first_x;
            }
            write(ufd, &input_event_buffer[i], sizeof(input_event_buffer[i]));
        }
    }
    else{
        for(int i = 0; i < buffer_index; i++){
            write(ufd, &input_event_buffer[i], sizeof(input_event_buffer[i]));
        }
    }
    buffer_index = 0;
    return;
}

// send all of the events we have been saving, including the latest SYN
static void act(int ufd, int correct_x){
    if(!sent_events){
        LOGI("act: adding tracking id\n");
        insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
    }
    LOGI("act: replaying buffer\n");
    replay_buffer(ufd, correct_x);
    return;
}

static void decide(int ufd){
    uint64_t d = now() - lastKbdTimestamp;
    int y_threshold = 30 * y_multiplier;
    int x_threshold = 30 * x_multiplier;


    //TODO handle tap events for tab here before the KB delay

    //500ms after typing ignore
    if(d < 500*1000) {
        reset();
        return;
    }

    // TODO: arrow key mode?

    if( (abs(first_y - latest_y) > y_threshold) || (abs(first_x - latest_x) > x_threshold)){
        LOGI("decide: acting\n");

        // TODO: correct y values to avoid activating the notification panel. this goes along with picking a proper multiplier. Also might need to do the same to avoid activating the switcher?
        act(ufd, 0);
        sent_events = 1;
    }
    return;
}

static void finalize(int ufd){
    insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, -1 );
    insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
    return;
}

int tapped = 0;
int was_tapped = 0;
int tap_x = 0;
int tap_y = 0;
static int64_t last_single_tap_time = 0;
// a touch starts with BTN_TOUCH DOWN. Store first x/y here
// store and act on every SYN
// when we see a difference > 60 start piping stored, and future syns until BTN_TOUCH UP
static void handle(int ufd, struct input_event e){
    /* LOGI("saw type = %d, code = %d, value = %d\n", e.type, e.code, e.value); */
    if(e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 1) {
        LOGI("BTN_TOUCH DOWN\n");
        touched = 1;
        buffer(e);

    }
    else if(touched){
        if(e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 0){
            if (sent_events){
                LOGI("BTN_TOUCH UP: sent events\n");
                buffer(e);
                act(ufd, 0);
                finalize(ufd);
                reset();
            }
            else{
                LOGI("BTN_TOUCH UP: resetting\n");
                if(was_tapped){
                    LOGI("entered was_tapped");
                    if( (now() - lastKbdTimestamp) > 1000*1000LL &&  (now() - last_single_tap_time) < 500*1000LL ){
                        LOGI("double tap time check passed");
                        if(isInRect(tap_x, tap_y, 1200, 1224, 600, 800)) {
                            LOGI("double tap first rect passed");
                            if(isInRect(latest_x, latest_y, 1200, 1224, 600, 800)) {
                                LOGI("Double tap on space key\n");
                                injectKey(ufd, KEY_TAB);
                            }

                        }
                        was_tapped = 0;
                        LOGI("setting was_tapped = 0");
                    }
                }
                else{
                    LOGI("setting was_tapped = 1");
                    was_tapped = 1;
                    tap_x = latest_x;
                    tap_y = latest_y;
                    last_single_tap_time = now();
                }

                reset();
            }
        }
        else if(e.type == EV_ABS && e.code == ABS_MT_POSITION_X) {
            buffer(e);
            if(first_x == -1){
                LOGI("setting first_x to %d\n", e.value);
                first_x = e.value;
                latest_x = e.value;
            }
            else{
                LOGI("setting latest_x to %d\n", e.value);
                latest_x = e.value;
            }
        }
        else if(e.type == EV_ABS && e.code == ABS_MT_POSITION_Y) {
            e.value = y_multiplier * e.value;
            buffer(e);
            if(first_y == -1){
                LOGI("setting first_y to %d\n", e.value);
                first_y = e.value;
                latest_y = e.value;
            }
            else{
                LOGI("setting latest_y to %d\n", e.value);
                latest_y = e.value;
            }
        }
        else if(e.type == EV_SYN && e.code == SYN_REPORT) {
            buffer(e);
            LOGI("SYN seen, deciding\n");
            decide(ufd);
        }
        else{
            //we have started a touch, but haven't decided to act yet.
            //save so we can replay if we decide to act
            buffer(e);
        }
    }
    return;
}

void *uinput_titan_monitor(void* ptr){
    (void)ptr;
    struct input_event kbe;

    int fd = open_ev("titan-uinput");
    if(fd<0){
        __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-META-MON-THREAD", "open failed!\n");
        return NULL;
    }
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-META-MON-THREAD", "open successfully!\n");

    while(1) {
        if(read(fd, &kbe, sizeof(kbe)) != sizeof(kbe)){
            break;
        }
        else{
            if(kbe.type == EV_KEY){
                __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-META-MON-THREAD", "read key code = %d, value = %d\n", kbe.code, kbe.value);
            }
        }
    }
    return NULL;
}

#define NUM_KEYS  4
int key_toggles[NUM_KEYS];
int key_locks[NUM_KEYS];
int ALT_INDEX = 0;
int SHIFT_INDEX = 1;
int FN_INDEX = 2;
int CTRL_INDEX = 3;
int KEY_CODES[NUM_KEYS] = {KEY_RIGHTALT, KEY_LEFTSHIFT, KEY_FN, KEY_LEFTCTRL};
// TODO: ctrl key not working, likely some interaction with the idcs/keymaps/keylayouts?, also need to fix arrow keys
void *keyboard_monitor(void* ptr) {
    int ufd = *(int*)ptr;
    /* (void)ptr; */

    // 1 second? TODO: no its not
    uint64_t lock_time = 1000*1000LL;
    struct input_event kbe;
    int key_index = -1;

    //aw9523-key
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "start\n");
    int fd = open_ev("aw9523-key");
    if(fd<0){
        __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "open failed!\n");
        return NULL;
    }
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "opened successfully\n");

    while(1) {
        if(read(fd, &kbe, sizeof(kbe)) != sizeof(kbe)){
            break;
        }
        else{

            if(kbe.type == EV_KEY){
                __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "read key code = %d, value = %d\n", kbe.code, kbe.value);
            }

            if(kbe.type == EV_KEY && kbe.value == 1){
                lastKbdTimestamp = now();
                if((kbe.code == KEY_LEFTALT || kbe.code == KEY_RIGHTALT) && kbe.value == 1){
                    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "read alt\n");
                    key_index = 0;
                }
                else if((kbe.code == KEY_LEFTSHIFT || kbe.code == KEY_RIGHTSHIFT) && kbe.value == 1){
                    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "read shift\n");
                    key_index = 1;
                }
                // we remap KEY_APPSELECT to KEY_FN in the .kl files, but that behavior is above the uinput events we are consuming so we have to watch for APPSELECT still
                else if(kbe.code == KEY_APPSELECT && kbe.value == 1){
                    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "read fn\n");
                    key_index = 2;
                }
                // we remap KEY_BACK to KEY_LEFTCTRL in the .kl files, but that behavior is above the uinput events we are consuming so we have to watch for BACK still
                else if(kbe.code == KEY_BACK && kbe.value == 1){
                    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "read control\n");
                    key_index = 3;
                }
                if(key_index != -1){
                    if(key_locks[key_index]){
                        key_locks[key_index] = 0;
                        __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "locked off %d\n", key_index);
                        injectKeyUp(ufd, KEY_CODES[key_index]);
                    }
                    else if(!key_toggles[key_index]){
                        __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "toggled on %d\n", key_index);
                        key_toggles[key_index] = 1;
                        injectKeyDown(ufd, KEY_CODES[key_index]);
                    }
                    else if(key_toggles[key_index]){
                        if(now() - lastKbdTimestamp < lock_time){
                            key_locks[key_index] = 1;
                            key_toggles[key_index] = 0;
                            __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "locked on %d\n", key_index);
                        }
                    }
                }
                else if(kbe.type == EV_KEY && kbe.value == 1){
                    // clear any toggles
                    for(int i = 0; i < NUM_KEYS; i++){
                        if(key_toggles[i]){
                            __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "toggled off %d\n", i);
                            key_toggles[i] = 0;
                            injectKeyUp(ufd, KEY_CODES[i]);
                        }
                    }
                }
                key_index = -1;
            }
        }
    }
    __android_log_print(ANDROID_LOG_INFO, "UINPUT-TITAN-KB-MON-THREAD", "error, returning\n");
    return NULL;
}


int main() {
    LOGI("start\n");
    int ufd = uinput_init();
    int origfd = original_input_init();

    LOGI("keyboard thread\n");
    pthread_t keyboard_monitor_thread;
    pthread_t keyboard_meta_monitor_thread;
    pthread_create(&keyboard_monitor_thread, NULL, keyboard_monitor, (void*) &ufd);
    pthread_create(&keyboard_meta_monitor_thread, NULL, uinput_titan_monitor, (void*) &ufd);
    LOGI("keyboard thread created\n");

    while(1) {
        struct input_event e;
        if(read(origfd, &e, sizeof(e)) != sizeof(e)) break;
        if(0) switch(e.type) {
            case EV_REL:
                ev_parse_rel(e);
                break;
            case EV_SYN:
                ev_parse_syn(e);
                break;
            case EV_KEY:
                ev_parse_key(e);
                break;
            case EV_ABS:
                ev_parse_abs(e);
                break;
            default:
                fprintf(stderr, "Unknown event type %d\n", e.type);
                break;
        };
        handle(ufd, e);
    }
}
