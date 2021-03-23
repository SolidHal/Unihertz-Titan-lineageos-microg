#define _GNU_SOURCE
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

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

static void insertEvent(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.code = code;
    e.value = value;
    write(fd, &e, sizeof(e));
}

static int uinput_init() {
    int fd = open("/dev/uinput", O_RDWR);

    struct uinput_user_dev setup = {
        .id = {
            .bustype = BUS_VIRTUAL,
            .vendor = 0xdead,
            .product = 0xbeaf,
            .version = 3,
        },
        .name = "titan uinput",
        .ff_effects_max = 0,
    };
    write(fd, &setup, sizeof(setup));

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_WHEEL);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, KEY_TAB);
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);

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

int injectKey(int ufd, int key) {
    insertEvent(ufd, EV_KEY, key, 1);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
    insertEvent(ufd, EV_KEY, key, 0);
    insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
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
        printf("single tap %d, %d, %d, %d; %lld %lld\n", x, y, y - oldY, x - oldX, duration, timeSinceLastSingleTap);

        if(duration < 120*1000LL && timeSinceLastSingleTap < 500*1000LL && d > 1000*1000LL) {
            printf("Got double tap\n");
            if(isInRect(oldX, oldY, 570, 721, 600, 900)) {
                printf("Double tap on space key\n");
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
        if( (y - oldY) > 60) {
            insertEvent(ufd, EV_REL, REL_WHEEL, 1);
            insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
            oldY = y;
            oldX = x;
            return;
        }
        if( (y - oldY) < -60) {
            insertEvent(ufd, EV_REL, REL_WHEEL, -1);
            insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
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
            insertEvent(ufd, EV_REL, REL_WHEEL, -1);
            insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
            oldY = y;
            oldX = x;
            return;
        }
    }
    if( (x - oldX) < -180) {
        //insertEvent(ufd, EV_REL, REL_WHEEL, -1);
        injectKey(ufd, KEY_LEFT);
        oldY = y;
        oldX = x;
        return;
    }
    if( (x - oldX) > 180) {
        //insertEvent(ufd, EV_REL, REL_WHEEL, 1);
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
