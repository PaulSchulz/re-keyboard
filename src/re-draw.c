// re-record
// Record pen strokes from reMarkable tablet

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/time.h>  // gettimeofday
#include <sys/select.h>

#include <sys/epoll.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <ctype.h>

#include <glib.h>
// GList - Double linked list

#include <stdbool.h>
#include <math.h>

#include "hershey-fonts.h"

//////////////////////////////////////////////////////////////////////////////
// Remarkable Data
typedef struct _segment_t {
    guint32 tv_sec;
    guint32 tv_usec;
    guint16 type;
    guint16 code;
    guint32 value;
} segment_t;

// Not used yet
typedef union _segment_u {
    int data[sizeof(segment_t)];
    segment_t segment;
} segment_u;

GList* stroke = NULL;
GList* runner;

typedef struct _metric_t {
    int x;
    int y;
    int width;
    int height;
    int segments;
} metric_t;

metric_t metric;

//////////////////////////////////////////////////////////////////////////////
void segment_set_time (segment_t* data) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    data->tv_sec  = tv.tv_sec;
    data->tv_usec = tv.tv_usec;
}

void segment_pen_down (segment_t* data) {
    data->type  =   1;
    data->code  = 320;
    data->value =   0;
}

void segment_pen_up (segment_t* data) {
    data->type  =   1;
    data->code  = 320;
    data->value =   0;
}

//////////////////////////////////////////////////////////////////////////////
void segment_write_data (segment_t* data) {
    /* fwrite(&data, 16, 1, fp_out); */

    putchar((data->tv_sec) & 0xFF);
    putchar((data->tv_sec / 0x100) & 0xFF);
    putchar((data->tv_sec / 0x10000) & 0xFF);
    putchar((data->tv_sec / 0x1000000) & 0xFF);

    putchar((data->tv_usec) & 0xFF);
    putchar((data->tv_usec / 0x100) & 0xFF);
    putchar((data->tv_usec / 0x10000) & 0xFF);
    putchar((data->tv_usec / 0x1000000) & 0xFF);

    putchar((data->type) & 0xFF);
    putchar((data->type / 0x100) & 0xFF);

    putchar((data->code) & 0xFF);
    putchar((data->code / 0x100) & 0xFF);

    putchar((data->value) & 0xFF);
    putchar((data->value / 0x100) & 0xFF);
    putchar((data->value / 0x10000) & 0xFF);
    putchar((data->value / 0x1000000) & 0xFF);

    // fprintf(stderr,"[%02X]",
    // (data->value>>(8*3)) & 0xFF);
}

//////////////////////////////////////////////////////////////////////////////
void segment_write_end (segment_t* data) {
    data->type  = 0;
    data->code  = 0;
    data->value = 0;
    segment_write_data(data);
}

#define NDATA 678
segment_t sdata[NDATA] =
{
#include "data.c"
};

GList* add_segment_to_path (GList* path, segment_t* segment) {
    return path;
}

// Calculate the metics (extents) for a path.
// TODO Test 'calculate_metric'
metric_t calculate_metric (GList* path) {
    GList* runner = path;
    segment_t* data;
    metric_t metric;
    metric.x        = 0;
    metric.y        = 0;
    metric.width    = 0;
    metric.height   = 0;
    metric.segments = 0;

    int minx = -1;
    int maxx = -1;
    int miny = -1;
    int maxy = -1;
    int count = 0;

    while (runner != NULL) {
        data = runner->data;

        switch (data->type) {
        case 0:
            switch (data->code) {
            case 0:
                if (data->value == 0) count++;
                break;
            default:
            }
            break;
        case 3:
            switch (data->code) {
            case 0:
                if (minx == -1) minx = data->value;
                if (maxx == -1) maxx = data->value;
                if (data->value < minx ) {
                    minx = data->value;
                } else if (data->value > maxx) {
                    maxx = data->value;
                }
                break;
            case 1:
                if (miny == -1) miny = data->value;
                if (maxy == -1) maxy = data->value;
                if (data->value < miny) {
                    miny = data->value;
                } else if (data->value > maxy) {
                    maxy = data->value;
                }
                break;
            default:
            }
        default:
        }

        runner = runner->next;
    }

    metric.x = minx;
    metric.y = miny;
    metric.width = maxx - minx;
    metric.height = maxy - maxx;
    // metric.segments = count;

    return metric;
}

segment_t* segment_translate(segment_t* data, int dx, int dy) {
    switch (data->type) {
    case 3:
        switch (data->code) {
        case 0:
            data->value = data->value + dx;
            break;
        case 1:
            data->value = data->value + dy;
            break;
        }
    }

    return data;
}

//////////////////////////////////////////////////////////////////////////////
#define EV_KEY         1
#define EV_ABS         3

#define BTN_TOOL_PEN 320
#define ABS_PRESSURE  24
#define ABS_DISTANCE  25
#define ABS_X          0
#define ABS_Y          1
#define ABS_TILT_X    26
#define ABS_TILT_Y    27

static void send_wacom_event(int type, int code, int value)
{
    segment_t segment;
    segment_set_time(&segment);
    segment.type  = type;
    segment.code  = code;
    segment.value = value;
    segment_write_data(&segment);
}

static void press_ui_button(int x, int y)
{
    // Pen down
    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 1);
    // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
    send_wacom_event(EV_ABS, ABS_PRESSURE, 0);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
    send_wacom_event(0, 0, 0);
    // finish_wacom_events();
    send_wacom_event(EV_ABS, ABS_X, y);
    send_wacom_event(EV_ABS, ABS_Y, x);
    send_wacom_event(EV_ABS, ABS_PRESSURE, 3288);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 0);
    send_wacom_event(EV_ABS, ABS_TILT_X, 0);
    send_wacom_event(EV_ABS, ABS_TILT_Y, 0);
    send_wacom_event(0, 0, 0);
    // finish_wacom_events();
    //send_wacom_event(EV_KEY, BTN_TOUCH, 1);
    send_wacom_event(0, 0, 0);
    // finish_wacom_events();
    // finish_wacom_events();
    usleep(10 * 1000);  // <---- If I remove this, strokes are missing.

    // Pen up
    send_wacom_event(EV_ABS, ABS_X, y);
    send_wacom_event(EV_ABS, ABS_Y, x);
    // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
    // finish_wacom_events();
    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 0);
    send_wacom_event(0, 0, 0);
    // finish_wacom_events();
}

//////////////////////////////////////////////////////////////////////////////
int cursor_x = 5000;
int cursor_y = 5000;
int limit_right = 10000;
int font_scale = 100;
bool wrap_ok = true;
static float mini_segment_length = 10.0;      // How far to move the pen while drawing

// Store points as floating point for calculations
typedef struct _point_t {
    float x;
    float y;
} point_t;

GList* prepend_point (GList* list, float x, float y){
    point_t* new_point  = malloc(sizeof(point_t));
    new_point->x=x;
    new_point->y=y;

    list = g_list_prepend(list, new_point);
    return list;
}

// Test Stroke Data
gint8 test_stroke_data[] = { 8, 18,   9,21,1,0,-1,-1,9,21,17,0,-1,-1,4,7,14,7,};

// Load stroke data
// Algorithm: Add points to front of list then reverse to order.
GList* stroke_load (const gint8* stroke_data,
                         int num_strokes,
                         GList* strokes) {

    for (int i=0; i<num_strokes; i++) {
        float raw_x = stroke_data[2*i + 0];
        float raw_y = stroke_data[2*i + 1];
        prepend_point(runner, raw_x, raw_y);
    }

    strokes = g_list_reverse(strokes);
    return strokes;
}

static void strokes_interp (const gint8* stroke_data,
                            int num_strokes,
                            GList* fstrokes)
{
    // This one actually works~
    float last_raw_x = -1;
    float last_raw_y = -1;

    // Stroke the character by scaling
    for (int i = 0; i < num_strokes; ++i)
    {
        float raw_x = stroke_data[2 * i + 0];
        float raw_y = stroke_data[2 * i + 1];
        if (raw_x != -1 || raw_y != -1)
        {
            if (last_raw_x != -1 || last_raw_y != -1)
            {
                float desired_dx = raw_x - last_raw_x;
                float desired_dy = raw_y - last_raw_y;
                float length = font_scale * sqrt(desired_dx * desired_dx + desired_dy * desired_dy);
                if (length > 0.0001)
                {
                    int subsegments = length / mini_segment_length - 1;
                    for (int i = 0; i < subsegments; ++i)
                    {
                        float t = (float)i / (float)subsegments;
                        float dx = t * desired_dx;
                        float dy = t * desired_dy;
                        float mod_x = last_raw_x + dx;
                        float mod_y = last_raw_y + dy;
                        // TODO Fix me
                        // fstrokes.push_back(mod_x);
// fstrokes.push_back(mod_y);
                        mod_x=mod_x;
                        mod_y=mod_y;
                    }
                }
            }
            // TODO Fix me
            // fstrokes.push_back(raw_x);
            // fstrokes.push_back(raw_y);
            raw_x = raw_x;
            raw_y = raw_y;
            last_raw_x = raw_x;
            last_raw_y = raw_y;
        }
        else
        {
            last_raw_x = -1;
            last_raw_y = -1;
            // fstrokes.push_back(-1);
            // fstrokes.push_back(-1);
        }
    }
}

static void wacom_char(char ascii_value, bool wrap_ok)
{
    int num_strokes = 0;
    int char_width = 0;
    char* current_font = "hershey";

    const gint8* stroke_data = get_font_char(current_font, ascii_value, &num_strokes, &char_width);
    if (!stroke_data)
        return;

    // Interpolate the strokes
    //static std::vector<float> fstrokes; // static to avoid constant allocation
    //fstrokes.clear();
    GList* fstrokes = NULL;
    strokes_interp(stroke_data, num_strokes, fstrokes);
    //num_strokes = fstrokes.size() >> 1;

    if (num_strokes > 0)
    {
        send_wacom_event(EV_KEY, BTN_TOOL_PEN, 1);
        // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
        send_wacom_event(EV_ABS, ABS_PRESSURE, 0);
        send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
        send_wacom_event(0, 0, 0);
        // finish_wacom_events();

        bool pen_down = false;
        float x = 0;
        float y = 0;
        for (int stroke_index = 0; stroke_index < num_strokes; ++stroke_index)
        {
            // TODO Fix measurements
            // float dx = fstrokes[2 * stroke_index + 0];
            // float dy = fstrokes[2 * stroke_index + 1];
            float dx = 0;
            float dy = 0;
            if (dx == -1 && dy == -1)
            {
                if (pen_down)
                {
                    send_wacom_event(EV_ABS, ABS_X, (int)y);
                    send_wacom_event(EV_ABS, ABS_Y, (int)x);
                    // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
                    send_wacom_event(EV_ABS, ABS_PRESSURE, 0);
                    send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
                    send_wacom_event(0, 0, 0);
                    // finish_wacom_events();
                    // finish_wacom_events();
                    usleep(1000);
                    // usleep(sleep_after_pen_up_ms * 1000);
                    pen_down = false;
                }
            }
            else
            {
                x = (float)dx * font_scale + cursor_x;
                y = (float)dy * font_scale + cursor_y;
                // finish_wacom_events();
                // usleep(sleep_each_stroke_point_ms * 1000);
                usleep(1000);
                send_wacom_event(EV_ABS, ABS_X, (int)y);
                send_wacom_event(EV_ABS, ABS_Y, (int)x);
                send_wacom_event(EV_ABS, ABS_PRESSURE, 3288);
                send_wacom_event(EV_ABS, ABS_DISTANCE, 0);
                send_wacom_event(EV_ABS, ABS_TILT_X, 0);
                send_wacom_event(EV_ABS, ABS_TILT_Y, 0);
                send_wacom_event(0, 0, 0);
                // finish_wacom_events();
                if (!pen_down)
                {
                    // send_wacom_event(EV_KEY, BTN_TOUCH, 1);
                    send_wacom_event(0, 0, 0);
                    // finish_wacom_events();
                    // usleep(sleep_after_pen_down_ms * 1000);  // <---- If I remove this, strokes are missing
                    usleep(1000);
                    pen_down = true;
                }
            }
        }
        //printf("\n");
        if (pen_down)
        {
            send_wacom_event(EV_ABS, ABS_X, (int)y);
            send_wacom_event(EV_ABS, ABS_Y, (int)x);
            // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
            send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
            // finish_wacom_events();
        }
        send_wacom_event(EV_KEY, BTN_TOOL_PEN, 0);
        send_wacom_event(0, 0, 0);
        // finish_wacom_events();
        // finish_wacom_events();
    }


    cursor_x += font_scale * char_width;
    // if (cursor_x > limit_right)
    // {
    //    if (wrap_ok)
    //        word_wrap();
    //    else
    //        new_line();
    //}

}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    // Remove buffer on stdout.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Draw a line
    int x;
    int y;

    x = 1000;
    y = 200;

    press_ui_button(50,12100);

    for (int j=0; j<10; j++) {

        for (int i=0; i<NDATA; i++) {
            segment_translate(&sdata[i],x,y);
            segment_write_data(&sdata[i]);
        }
        usleep(100);

    }

    const char* current_font = "hershey";
    int ascii_value = 0x41;
    int num_strokes;
    int char_width;
    const gint8* stroke_data = get_font_char(current_font, ascii_value, &num_strokes, &char_width);
    stroke_data = stroke_data;

    wacom_char(ascii_value, wrap_ok);


    exit(0);

}
