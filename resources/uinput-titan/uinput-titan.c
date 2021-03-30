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
static float y_multiplier = 1.5;
// TODO: pick a proper multiplier. best so far is 1.5
struct input_event event_synth_buffer[2048];
int synth_buffer_index = 0;
struct input_event last_y;
int last_y_valid = 0;


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
    synth_buffer_index = 0;
    last_y_valid = 0;
    return;
}

static void synth_buffer(struct input_event event){
    event_synth_buffer[synth_buffer_index] = event;
    synth_buffer_index++;
}

/**
 * Compute a fraction of given struct timeval.
 *
 * \param div The integer value to divide by.
 * \param tv The timevalue to divide.
 * \param result Contains (1 / mult) * tv on return.
 */
void tv_divide(const unsigned long div, const struct timeval *tv, struct timeval *result)
{
    uint64_t x = ((uint64_t)tv->tv_sec * 1000 * 1000 + tv->tv_usec) / div;
    result->tv_sec = x / 1000 / 1000;
    result->tv_usec = x % (1000 * 1000);
    return;
}

static void tv_diff_divide(const unsigned long div, struct timeval ya, struct timeval yb, struct timeval *ret){
    // returns a struct timeval of diff(yb -ya) / div
    struct timeval timediff;
    timersub(&yb, &ya, &timediff);
    tv_divide(div, &timediff, ret);
    return;
}

/*
  creates a sequence of events that looks like:
  /dev/input/event6: EV_ABS       ABS_MT_TRACKING_ID   00000000
  /dev/input/event6: EV_ABS       ABS_MT_POSITION_X    000004ec
  /dev/input/event6: EV_ABS       ABS_MT_POSITION_Y    00000243
  /dev/input/event6: EV_ABS       ABS_MT_TOUCH_MAJOR   00000006
  /dev/input/event6: EV_ABS       ABS_MT_TOUCH_MINOR   00000005
  /dev/input/event6: EV_SYN       SYN_MT_REPORT        00000000
  /dev/input/event6: EV_SYN       SYN_REPORT           00000000
 */
static void synth_scroll_packet(int y, int x, struct timeval time){
    LOGI("synthing a scroll packet at time           [%ld.%ld],                 y = %d\n", time.tv_sec, time.tv_usec, y);
    struct input_event tmp;
    // Tracking ID
    tmp.time = time;
    tmp.type = EV_ABS;
    tmp.code = ABS_MT_TRACKING_ID;
    tmp.value = 0;
    synth_buffer(tmp);
    // POS_X
    tmp.time = time;
    tmp.type = EV_ABS;
    tmp.code = ABS_MT_POSITION_X;
    tmp.value = x;
    synth_buffer(tmp);
    // POS_Y
    tmp.time = time;
    tmp.type = EV_ABS;
    tmp.code = ABS_MT_POSITION_Y;
    tmp.value = y;
    synth_buffer(tmp);
    // TOUCH_MAJOR
    tmp.time = time;
    tmp.type = EV_ABS;
    tmp.code = ABS_MT_TOUCH_MAJOR;
    tmp.value = 3;
    synth_buffer(tmp);
    // TOUCH_MINOR
    tmp.time = time;
    tmp.type = EV_ABS;
    tmp.code = ABS_MT_TOUCH_MINOR;
    tmp.value = 2;
    synth_buffer(tmp);
    // SYN_MT_REPORT
    tmp.time = time;
    tmp.type = EV_SYN;
    tmp.code = SYN_MT_REPORT;
    tmp.value = 0;
    synth_buffer(tmp);
    // SYN_REPORT
    tmp.time = time;
    tmp.type = EV_SYN;
    tmp.code = SYN_REPORT;
    tmp.value = 0;
    synth_buffer(tmp);
    return;
}

// pass in the last y input_event seen when we last replayed the buffer
static void create_synth_buffer(){

    int event_packet_length = 7;
    int synth_threshold = 5; // how far apart do any two packets need to be to synthesize packets between them

    int last_packet = 0;
    int diff = 0;
    int div = 0;

    struct input_event event_ya;
    struct input_event event_yb;
    struct input_event current_event;
    struct input_event prev_x;

    struct timeval timediff;

    int synth_y = 0;
    struct timeval synth_time;

    for(int i = 0; i< buffer_index; i++){
        current_event = input_event_buffer[i];
        synth_buffer(current_event);

        if(last_y_valid){
            // if we have last_y_valid then we are in the middle of touch_down -> touch_up
            // but have already emptied the last buffer.
            // our current event type should be a ABS_MT_TRACKING_ID
            event_ya = last_y;
            prev_x = input_event_buffer[i+1];
            event_yb = input_event_buffer[i+2];
        }

        if(current_event.code == ABS_MT_POSITION_Y){
            event_ya = current_event;
            if(i+event_packet_length < buffer_index){
                prev_x = input_event_buffer[i-1];
                event_yb = input_event_buffer[i+event_packet_length];
            }
            else{
                // we are on the last packet, nothing left to do
                last_packet = 1;
            }
        }

        if( last_y_valid || (current_event.code == SYN_REPORT && !last_packet)){
            last_y_valid = 0;
            LOGI("synthing a scroll packet between times a = [%ld.%ld], b = [%ld.%ld], ya = %d, yb = %d\n", event_ya.time.tv_sec, event_ya.time.tv_usec, event_yb.time.tv_sec, event_yb.time.tv_usec, event_ya.value, event_yb.value);
            diff = event_ya.value - event_yb.value;
            div =  abs(diff)/synth_threshold;
            if ( abs(diff) >= synth_threshold){
                tv_diff_divide((unsigned long) div, event_ya.time, event_yb.time, &timediff);
                LOGI("synthing timediff = [%ld.%ld]\n", timediff.tv_sec, timediff.tv_usec);
                synth_y = event_ya.value;
                synth_time = event_ya.time;
                for(int j = 0; j < div ; j++){
                    if (diff > 0){
                        // up swipe, decreasing value from a -> b
                        // timestamps should be evenly distributed between
                        // event_ya.timeval and event_yb.timeval
                        // so diff % synth_threshold divisions of event_yb.timeval - event_ya.timeval
                        synth_y = synth_y - synth_threshold;
                        timeradd(&timediff, &synth_time, &synth_time);
                        synth_scroll_packet(synth_y, prev_x.value, synth_time);
                    }
                    else if (diff < 0){
                        // down swipe, increasing value from a -> b
                        synth_y = synth_y + synth_threshold;
                        timeradd(&timediff, &synth_time, &synth_time);
                        synth_scroll_packet(synth_y, prev_x.value, synth_time);
                    }
                }
            }
        }
    }
}

static void replay_buffer(int ufd, int correct_x, int synth_swipe){
    LOGI("replaying buffer, index = %d, correct_x = %d, synth_swipe = %d\n", buffer_index, correct_x, synth_swipe);
    if (synth_swipe){
        // scrolling is rough since we have a multiplier. position updates end up roughly 50 apart with 1.5 multiplier, vs 1-2 apart on the touch screen
        // synthesize additional events to create smoother scrolling
        create_synth_buffer();
        for(int i = 0; i < synth_buffer_index; i++){
            if(event_synth_buffer[i].type == EV_ABS && event_synth_buffer[i].code == ABS_MT_POSITION_Y){
                last_y = event_synth_buffer[i];
            }
            write(ufd, &event_synth_buffer[i], sizeof(event_synth_buffer[i]));
        }
        last_y_valid = 1;
    }
    else if (correct_x){
        for(int i = 0; i < buffer_index; i++){
            if(input_event_buffer[i].code == ABS_MT_POSITION_X){
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
static void act(int ufd, int correct_x, int synth_swipe){
    if(!sent_events){
        LOGI("act: adding tracking id\n");
        insertEvent(ufd, EV_ABS, ABS_MT_TRACKING_ID, 0 );
    }
    LOGI("act: replaying buffer\n");
    replay_buffer(ufd, correct_x, synth_swipe);
    return;
}

static void decide(int ufd){
    uint64_t d = now() - lastKbdTimestamp;
    int y_threshold = 40 * y_multiplier;


    //TODO handle tap events for tab here before the KB delay

    //500ms after typing ignore
    if(d < 500*1000) {
        reset();
        return;
    }
    if( (abs(first_y - latest_y) > y_threshold && buffer_index >= 14)){
        LOGI("decide: acting\n");

        // TODO: correct y values to avoid activating the notification panel. this goes along with picking a proper multiplier. Also might need to do the same to avoid activating the switcher?
        act(ufd, 1, 0);
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
                act(ufd, 0, 0);
                finalize(ufd);
                reset();
            }
            else{
                LOGI("BTN_TOUCH UP: resetting\n");
                reset();
            }
        }
        else if(e.type == EV_ABS && e.code == ABS_MT_POSITION_X) {
            buffer(e);
            if(first_x == -1){
                LOGI("setting first_x\n");
                first_x = e.value;
                latest_x = e.value;
            }
            else{
                LOGI("setting latest_x\n");
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


void *keyboard_monitor(void* ptr) {
    (void) ptr;
    //aw9523-key
    int fd = open_ev("aw9523-key");

    while(1) {
        struct input_event kbe;
        if(read(fd, &kbe, sizeof(kbe)) != sizeof(kbe)) break;
        // TODO: USE THIS TO MAKE ALT, SHIFT, ETC keys toggle instead of requireing them to be held down?
        // basic idea is when we see a ALT_* or SHIFT_* release to send a ALT_* or SHIFT_* press, save state, then send the release when
        // we see another ALT_* or SHIFT_*
        lastKbdTimestamp = now();
    }
    return NULL;
}

int main() {
    LOGI("start\n");
    int ufd = uinput_init();
    int origfd = original_input_init();

    LOGI("keyboard thread\n");
    pthread_t keyboard_monitor_thread;
    pthread_create(&keyboard_monitor_thread, NULL, keyboard_monitor, NULL);

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
