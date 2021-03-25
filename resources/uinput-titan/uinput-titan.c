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
#include <android/log.h>

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

struct input_event e;

static void insertEvent(int fd, int type, int code, int value) {
    memset(&e, 0, sizeof(e));
    gettimeofday(&e.time, NULL);
    e.type = type;
    e.code = code;
    e.value = value;
    write(fd, &e, sizeof(e));
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
                    .maximum = 1440,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = 1440,
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
                    .maximum = 1440,
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
    ioctl(fd, UI_SET_KEYBIT, KEY_TAB);
    ioctl(fd, UI_SET_KEYBIT, KEY_PAGEUP);
    ioctl(fd, UI_SET_KEYBIT, KEY_PAGEDOWN);

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
    insertEvent(ufd, EV_KEY, key, 1);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
    return 0;
}

int injectKeyUp(int ufd, int key){
    insertEvent(ufd, EV_KEY, key, 0);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
    return 0;
}

int injectKey(int ufd, int key) {
    injectKeyDown(ufd, key);
    injectKeyUp(ufd, key);
    return 0;
}

int injectAbsEvent(int ufd, int x, int y, bool first, bool last){
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
    if(last){
        insertEvent(ufd, EV_KEY, BTN_TOUCH, 0 );
        insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, -1 );
        insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
        insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
    }
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
    // map from 720 to 1440
    ystart = 2*ystart;
    yend = 2*yend;

    // determine if it is a vertical or horizontal swipe

    bool vertical = false;
    bool positive = false
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
    }

    LOGI("SENDING SWIPE\n");
    insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
    insertEvent(ufd, EV_KEY, BTN_TOUCH, 1 );
    insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, xstart );
    insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, ystart );
    insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
    insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );
    insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );

    if(ystart < yend && xstart < xend){
        for(int i = xstart+1; i <= xend; i += 10){
            for(int j = ystart+1; j <= yend; j += 10){

                insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, i );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, j );

                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );

                insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
                insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
            }
        }
    }
    else if(ystart < yend && xstart > xend){
        for(int i = xstart-1; i >= xend; i -= 2){
            for(int j = ystart+1; j <= yend; j += 2){

                insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, i );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, j );

                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );

                insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
                insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
            }
        }
    }
    else if(ystart > yend && xstart > xend){
        for(int i = xstart-1; i >= xend; i -= 2){
            for(int j = ystart-1; j >= yend; j -= 2){

                insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, i );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, j );

                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );

                insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
                insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
            }
        }
    }
    else if(ystart > yend && xstart < xend){
        for(int i = xstart+1; i <= xend; i += 2){
            for(int j = ystart-1; j >= yend; j -= 2){

                insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_X, i );
                insertEvent(ufd, EV_ABS, ABS_MT_POSITION_Y, j );

                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1 );
                insertEvent(ufd, EV_ABS, ABS_MT_TOUCH_MINOR, 1 );

                insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
                insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
            }
        }
    }

    insertEvent(ufd, EV_KEY, BTN_TOUCH, 0 );
    insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, (int)-1 );
    insertEvent(ufd, EV_SYN, SYN_MT_REPORT, 0 );
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0 );
    LOGI("SENT SWIPE\n");

    return 0;
}

static int wasTouched, oldX, oldY, nEventsInSwipe;
static int64_t startT, lastSingleTapT;
static int lastSingleTapX, lastSingleTapY, lastSingleTapDuration;
//touchpanel resolution is 1440x720
static void decide(int ufd, int touched, int x, int y) {
    uint64_t d = now() - lastKbdTimestamp;

    if(wasTouched && !touched && nEventsInSwipe == 0) {
        int64_t duration = now() - startT;
        int64_t timeSinceLastSingleTap = now() - lastSingleTapT;
        LOGI("single tap %d, %d, %d, %d %" PRId64 " %" PRId64 "\n", x, y, y - oldY, x - oldX, duration, timeSinceLastSingleTap);

        if(duration < 120*1000LL && timeSinceLastSingleTap < 500*1000LL && d > 1000*1000LL) {
            LOGI("Got double tap\n");
            if(isInRect(oldX, oldY, 570, 721, 600, 900)) {
                LOGI("Double tap on space key\n");
                injectKey(ufd, KEY_TAB);
            }
        }

        lastSingleTapX = oldX;
        lastSingleTapY = oldY;
        lastSingleTapT = startT;
        lastSingleTapDuration = duration;
    }
    if(!touched) {
        wasTouched = 0;
        return;
    }
    if(!wasTouched && touched) {
        oldX = x;
        oldY = y;
        startT = now();
        wasTouched = touched;
        nEventsInSwipe = 0;
        return;
    }

    //500ms after typing ignore
    if(d < 500*1000) {
        oldX = x;
        oldY = y;
        return;
    }

    nEventsInSwipe++;
    printf("%d, %d, %d, %d, %d\n", touched, x, y, y - oldY, x - oldX);
    //2/3 width right side is used for scrolling
    if(x > 300) {
        if( abs(y - oldY) > 60) {
            injectSwipe(ufd, oldX, oldY, x, y);
            oldY = y;
            oldX = x;
            return;
        }
    } else {
        //1/3 left side is used to trigger notifications
        if( (y - oldY) > 280) {
            system("cmd statusbar expand-notifications");
            oldY = y;
            oldX = x;
            return;
        }
        if( (y - oldY) < -280) {
            system("cmd statusbar collapse");
            oldY = y;
            oldX = x;
            return;
        }
    }
    if( (x - oldX) < -180) {
        injectKey(ufd, KEY_LEFT);
        oldY = y;
        oldX = x;
        return;
    }
    if( (x - oldX) > 180) {
        injectKey(ufd, KEY_RIGHT);
        oldY = y;
        oldX = x;
        return;
    }
}

void *keyboard_monitor(void* ptr) {
    (void) ptr;
    //aw9523-key
    int fd = open_ev("aw9523-key");

    while(1) {
        struct input_event e;
        if(read(fd, &e, sizeof(e)) != sizeof(e)) break;
        // TODO: USE THIS TO MAKE ALT, SHIFT, ETC keys toggle instead of requireing them to be held down?
        // basic idea is when we see a ALT_* or SHIFT_* release to send a ALT_* or SHIFT_* press, save state, then send the release when
        // we see another ALT_* or SHIFT_*
        lastKbdTimestamp = now();
    }
    return NULL;
}

int main() {
    int ufd = uinput_init();
    int origfd = original_input_init();

    pthread_t keyboard_monitor_thread;
    pthread_create(&keyboard_monitor_thread, NULL, keyboard_monitor, NULL);

    int currentlyTouched = 0;
    int currentX = -1;
    int currentY = -1;
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
        if(e.type == EV_KEY && e.code == BTN_TOUCH) {
            currentlyTouched = e.value;
        }
        if(e.type == EV_ABS && e.code == ABS_MT_POSITION_X) {
            currentX = e.value;
        }
        if(e.type == EV_ABS && e.code == ABS_MT_POSITION_Y) {
            currentY = e.value;
        }
        if(e.type == EV_SYN && e.code == SYN_REPORT) {
            decide(ufd, currentlyTouched, currentX, currentY);
        }
    }
}
