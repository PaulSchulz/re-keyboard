//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Used for nanosleep
#include <linux/input.h>
#include <stdarg.h> // Used va_list for variable function arguments
#include <fcntl.h>

#include "fonts.h"

//////////////////////////////////////////////////////////////////////////////
// #include "debug.h"
// Utilities
char debug_str[128] = "";
char verbose_mode = 1;

void debug(char* string) {
    if (verbose_mode == 1) {
        printf(string);
        fflush(stdout);
    }
    return;
}

void debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("DEBUG ");
    vprintf(format, args);
    va_end(args);
}

void dev_sleep (int sec, int nsec) {
    struct timespec sleep_time;
    sleep_time.tv_sec = sec;
    sleep_time.tv_nsec = nsec;

    debug_print("Sleeping for %d.%09ds.\n", sec, nsec);
    int result = nanosleep(&sleep_time, NULL);
    if (result == 0) {
        debug_print("Done sleeping.\n");
    } else {
        debug_print("Sleep was interrupted.\n");
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////
// Device handling
char* pen_device_path = "/dev/input/event1";
static int device_wacom = -1;

static void open_device(void) {
    if (device_wacom == -1){
        device_wacom = open(pen_device_path, O_WRONLY);
        if (device_wacom >= 0) {
            debug_print("  Connected to pen input device for writing.\n");
        } else {
            debug_print("  Failed to connect to pen input device.\n");
        }
    }

}

//////////////////////////////////////////////////////////////////////////////
// Writing Events
static void write_wacom_event () {
    //    write(device_wacom,
    //      &pending_events[0],
    //      pending_events.size() * sizeof(pending_events[0]));
}

static void finish_wacom_events()
{
    /*
      if (pending_events.size())
      {
      // write(device_wacom, &pending_events[0], pending_events.size() * sizeof(pending_events[0]));
      fsync(device_wacom);
      pending_events.clear();
      }
    */
}

static void send_wacom_event(int type, int code, int value)
{
    /*
      struct input_event evt;
      gettimeofday(&evt.time, NULL);
    evt.type = type;
    evt.code = code;
    evt.value = value;
    pending_events.push_back(evt);
    finish_wacom_events();
    */
}

//////////////////////////////////////////////////////////////////////////////
static void do_main_loop() {
    while (1) {
        debug_print("do_main_loop\n");
        dev_sleep(1,0);
    }
}

static void initialize() {
    debug_print("initialize\n");
    debug_print("Version:\n");

    dev_sleep(1,0);

    debug_print("Opening pen device: %s\n", pen_device_path);
    open_device();

    // Self Test
    int     char_value = 0;
    int     segments   = 0;
    int     width      = 0;
    const int8_t* char_data  = NULL;
    char*   font_name  = "hershey";

    if (device_wacom >= 0) {
        char_value = 65;
        char_data = get_font_char(font_name, char_value, &segments, &width);
        debug_print("Character: %20s %d (%c) %d %d\n",
               font_name,
               char_value, char_value,
               segments,
               width);

        struct input_event evt;
        for(int i=0; i<segments; i++){
            debug_print("  (%d,%d)\n",
                        char_data[2*i], char_data[2*i+1]);

            // Put somethign here!!

        }

    }

}

int main(int argc, char *argv[]) {
    initialize();
    while(1) {
        do_main_loop();
    }
    return 0;

}
