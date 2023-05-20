//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Used for 'nanosleep'
#include <unistd.h> // Used for 'write'
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

    // debug_print("Sleeping for %d.%09ds.\n", sec, nsec);
    int result = nanosleep(&sleep_time, NULL);
    if (result == 0) {
        // debug_print("Done sleeping.\n");
    } else {
        // debug_print("Sleep was interrupted.\n");
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

    // Pause
    dev_sleep(0,100000);
}

static void send_wacom_event(int type, int code, int value)
{
    struct input_event evt;
    gettimeofday(&evt.time, NULL);
    // evt.time.tv_sec = 0;
    // evt.time.tv_usec = 0;
    evt.type = type;
    evt.code = code;
    evt.value = value;
    // pending_events.push_back(evt);
    // finish_wacom_events();

    debug_print("EVENT %d.%06d %04x %04x %08x\n",
                evt.time.tv_sec,
                evt.time.tv_usec,
                evt.type,
                evt.code,
                evt.value);

    int result = write(device_wacom, &evt, sizeof(evt));
    if (result != 16) {
        debug_print("EVENT SIZE: %d\n", result);
    }
}

//////////////////////////////////////////////////////////////////////////////
#define RECORD_SIZE 16

void data_test(){
    debug_print("Opening data\n");
    FILE *data;
    char buffer[RECORD_SIZE];

    data = fopen("data", "rb");

    if (data == NULL) {
        printf("Failed to open the file.\n");
        return;
    }

    int count = 0;

    while (fread(buffer, sizeof(char), RECORD_SIZE, data) == RECORD_SIZE) {

        count++;
        // Process the record in the buffer
        write(device_wacom, buffer, sizeof(buffer));
        fsync(device_wacom);

        if (count == 40) {
            dev_sleep(5,0);
        }

        // Print the record for demonstration purposes
        printf("%04d: ", count);
        for (int i = 0; i < RECORD_SIZE; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        printf("\n");
    }

    if (!feof(data)) {
        printf("Failed to read from the file.\n");
    }

    fclose(data);
}


void font_test() {

    // Draw a line by sending events.
    // DOES NOT WORK
    debug_print("Line\n");
    debug_print("\n");
    // Strokes

    // Pen down
    int x0 = 0x2000;
    int y0 = 0x2000;
    int x = x0;
    int y = y0;

    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 1);
    send_wacom_event(EV_ABS, ABS_X, x);
    send_wacom_event(EV_ABS, ABS_Y, y);
    send_wacom_event(EV_ABS, ABS_PRESSURE, 0x00);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 0x4e);
    send_wacom_event(EV_ABS, ABS_TILT_X, 0);
    send_wacom_event(EV_ABS, ABS_TILT_Y, 0);
    send_wacom_event(0, 0, 0);
    finish_wacom_events();

    for(int i=0; i<100; i++){
        int x = x0 + i*10;
        int y = y0;
        send_wacom_event(EV_ABS, ABS_X, x);
        send_wacom_event(EV_ABS, ABS_Y, y);
        send_wacom_event(EV_ABS, ABS_DISTANCE, 0x4a);
        send_wacom_event(0, 0, 0);
        finish_wacom_events();
        fsync(device_wacom);
        dev_sleep(0,100000000);

    }
    // Pen Up
    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 0);
    send_wacom_event(0, 0, 0);
    finish_wacom_events();
    fsync(device_wacom);
    dev_sleep(0,100000000);
}

void font_data_test() {
    int     char_value = 0;
    int     segments   = 0;
    int     width      = 0;
    const int8_t* char_data = NULL;
    char*         font_name = "hershey";

    char_value = 65;
    char_data = get_font_char(font_name, char_value, &segments, &width);
    debug_print("Character: %20s %d (%c) %d %d\n",
                font_name,
                char_value, char_value,
                segments,
                width);
}
//////////////////////////////////////////////////////////////////////////////
struct text_gc_t {
    uint32_t x0;
    uint32_t y0;
};

void write_character(struct text_gc_t* text_gc, char ch){
    char* data_path = "font/glyph_%02d.dat";

    debug_print(data_path, ch);

    return;
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

    // font_data_test();

    if (device_wacom >= 0) {
        struct input_event evt;

        int x0 = 1800;
        int y0 =  200;
        int scale = 1.0;

        int a0 = 1000;
        int b0 = 1000;
        int a = 0;
        int b = 0;

        int x = b + b0 + x0;
        int y = a + a0 + y0;

        int x_char_offset = 2000;
        int y_char_offset =  200;

        // https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h

        // data_test()

        ///////////////////////////////////////////////////////////////////
        int count;
        FILE *data;
        char buffer[sizeof(struct input_event)];
        data = fopen("font/glyph_41.dat", "rb");

        if (data == NULL) {
            printf("Failed to open the file.\n");
            return;
        }

        while (fread(buffer, sizeof(char), RECORD_SIZE, data) == RECORD_SIZE) {
            // Process the record in the buffer
            uint16_t type;
            uint16_t code;
            uint32_t value;

            type = (uint16_t)(buffer[8]) | (uint16_t)(buffer[9]) << 8;
            code = (uint16_t)(buffer[10]) | (uint16_t)(buffer[11]) << 8;
            value = (uint32_t)(buffer[12])
                | (uint32_t)(buffer[13]) << 8
                | (uint32_t)(buffer[14]) << 16
                | (uint32_t)(buffer[15]) << 24;

            if (code == ABS_X) {
                // Data conditioning
                value = value - x_char_offset;
                value = value / 8;

                // Scale
                value = value * scale;
                // Reposition
                value = value + x;
            }

            if (code == ABS_Y) {
                // Data conditioning
                value = value - y_char_offset;
                value = value / 8;

                // Scale
                value = value * scale;
                // Reposition
                value = value + y;

            }

            send_wacom_event(type,code,value);

            // write(device_wacom, buffer, sizeof(buffer));
            fsync(device_wacom);
            // dev_sleep(0,50000);

        }

        if (!feof(data)) {
            printf("Failed to read from the file.\n");
        }

        fclose(data);
        debug_print("\n");

        // font_test();

    } else {
        debug_print("Pen device not open for writing\n.");
    }

}

int main(int argc, char *argv[]) {
    initialize();
    exit(0);

    while(1) {
        do_main_loop();
    }
    return 0;

}
