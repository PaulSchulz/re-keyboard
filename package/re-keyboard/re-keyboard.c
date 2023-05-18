//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h> // Used for nanosleep
#include "fonts.h"

//////////////////////////////////////////////////////////////////////////////
// #include "debug.h"
char debug_str[128] = "";
char verbose_mode = 1;

void debug(char* string) {
    if (verbose_mode == 1) {
        printf(string);
        fflush(stdout);
    }
    return;
}

void dev_sleep (int sec, int nsec) {
    struct timespec sleep_time;
    sleep_time.tv_sec = sec;  // Sleep for 2 seconds
    sleep_time.tv_nsec = nsec;  // Sleep for an additional 500 milliseconds (500 million nanoseconds)

    int result = nanosleep(&sleep_time, NULL);
    if (result == 0) {
        debug("Done sleeping.\n");
    } else {
        debug("Sleep was interrupted.\n");
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////
char* pen_device_path = "/dev/input/event1";
static int device_wacom = -1;

static void open_device(void) {
    if (device_wacom == -1){
        device_wacom = open(pen_device_path, O_WRONLY);
        if (device_wacom >= 0) {
            debug("  Connected to pen input device.\n");
        } else {
            debug("  Failed to connect to pen input device.\n");
        }
    }

}

//////////////////////////////////////////////////////////////////////////////
static void do_main_loop() {
    while (1) {
        debug("do_main_loop");
        dev_sleep(1,0);
    }
}

static void initialize() {
    debug("initialize");
    debug("Version");

    dev_sleep(1,0);

    debug("Opening pen device.\n");
    open_device();
}

int main() {
    initialize();
    do_main_loop();
    return 0;
}
