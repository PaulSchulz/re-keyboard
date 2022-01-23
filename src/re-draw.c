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
    fprintf(stderr,"DEBUG: segment_write_data() - %d,%d,%d\n",
            data->type,
            data->code,
            data->value);
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
//    fprintf(stderr,"DEBUG: send_wacom_event() %d,%d,%d\n",type,code,value);

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
    usleep(10 * 1000);  // <---- If I remove this, strokes are missing.

    // Pen up
    send_wacom_event(EV_ABS, ABS_X, y);
    send_wacom_event(EV_ABS, ABS_Y, x);
    // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
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

GList* insert_before_point (GList* list, GList* element, float x, float y){
    point_t* new_point  = malloc(sizeof(point_t));
    new_point->x=x;
    new_point->y=y;

    list = g_list_insert_before(list, element, new_point);
    return list;
}

GList* append_point (GList* list, float x, float y){
    point_t* new_point  = malloc(sizeof(point_t));
    new_point->x=x;
    new_point->y=y;

    list = g_list_append(list, new_point);
    return list;
}

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
GList* stroke_load (GList*       strokes,
                    const gint8* stroke_data,
                    int          num_strokes) {

    fprintf(stderr, "DEBUG: stroke_load()\n");
    fprintf(stderr, "DEBUG:   num_strokes: %d\n", num_strokes);

    for (int i=0; i<num_strokes; i++) {
        float raw_x = stroke_data[2*i + 0];
        float raw_y = stroke_data[2*i + 1];
        runner = prepend_point(runner, raw_x, raw_y);

        fprintf(stderr,"DEBUG:   (raw_x,raw_y)=(%f,%f)\n",
                raw_x,
                raw_y);

        fprintf(stderr,"DEBUG:   size: %d\n", g_list_length(strokes));
        fprintf(stderr,"DEBUG:   size: %d\n", g_list_length(runner));
    }

    strokes = g_list_reverse(runner);
                        return strokes;
}

GList* stroke_scale (GList* strokes,
                     float  xscale,
                     float  yscale) {

    GList* runner;
    point_t* data;

    runner = strokes;
    while(runner != NULL){
        data = (point_t*)runner->data;
        if (data->x != -1 && data->y != -1) {
            data->x = xscale*(data->x);
            data->y = yscale*(data->y);
        }

        runner = runner->next;
    }

    return strokes;
}

GList* stroke_translate (GList* strokes,
                         float  xshift,
                         float  yshift) {

    GList* runner;
    point_t* data;

    runner = strokes;
    while(runner != NULL){
        data = (point_t*)runner->data;
        if (data->x != -1 && data->y != -1) {
            data->x = xshift+(data->x);
            data->y = yshift+(data->y);
        }
        runner = runner->next;
    }

    return strokes;
}

// TODO stroke_interp needs to be tested.
GList* stroke_interpolate (GList* stroke) {
    fprintf(stderr, "DEBUG: stroke_interpolate()\n");

    // This one actually works~
    float last_raw_x = -1;
    float last_raw_y = -1;

    point_t* point;

    GList* runner = stroke;
    while (runner != NULL) {
        point = (point_t*)runner->data;

        float raw_x = point->x;
        float raw_y = point->y;
        if (raw_x != -1 || raw_y != -1) {
            fprintf(stderr,"DEBUG: valid_point (%f,%f)\n", raw_x, raw_y);
            // Segment end is defined.
            if (last_raw_x != -1 || last_raw_y != -1) {
                fprintf(stderr,"DEBUG:   valid_previous_point (%f,%f)\n", last_raw_x, last_raw_y);
                // Segment start and end are defined.
                float desired_dx = raw_x - last_raw_x;
                float desired_dy = raw_y - last_raw_y;
                float length = sqrt(desired_dx * desired_dx + desired_dy * desired_dy);
                // float length = font_scale * sqrt(desired_dx * desired_dx + desired_dy * desired_dy);

                fprintf(stderr, "DEBUG:   length: %f\n", length);

                if (length > 0.0001) {
                    // If length is larger then minimum step size then add
                    // interpolated values.
                    int subsegments = length / mini_segment_length - 1;

                    fprintf(stderr,"DEBUG:   interpolation required, %d\n", subsegments);

                    for (int i = 0; i < subsegments; ++i) {
                        float t = (float)i / (float)subsegments;
                        float dx = t * desired_dx;
                        float dy = t * desired_dy;
                        float mod_x = last_raw_x + dx;
                        float mod_y = last_raw_y + dy;

                        // Insert new point into list
                        runner = insert_before_point(runner, runner, mod_x, mod_y);
                    }
                }
            } else {
                fprintf(stderr,"DEBUG:   start(-1,-1) previous point\n");
                // Segment end is not defined.
                // End of pen stroke.
                // Move to next point.
            }
        } else {
            fprintf(stderr,"DEBUG: end(-1,-1) point\n");
        }
        last_raw_x = raw_x;
        last_raw_y = raw_y;

        runner = runner->next;
    }
    return stroke;
}

// TODO stroke_interp needs to be tested.
GList* stroke_write (GList* stroke) {
    fprintf(stderr, "DEBUG: stroke_write()\n");
    fprintf(stderr, "DEBUG:   length: %d\n", g_list_length(stroke));

    if (g_list_length(stroke) == 0){
        return stroke;
    }

    // Initial strin, pen down
    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 1);
    // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
    send_wacom_event(EV_ABS, ABS_PRESSURE, 0);
    send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
    send_wacom_event(0, 0, 0);
    // finish_wacom_events();

    GList* runner;
    runner = stroke;
    point_t* data;

    float x_prev = -1;
    float y_prev = -1;
    data = runner->data;
    float x = data->x;
    float y = data->y;

    // TODO Remove when no longer required
    x=x;
    y=y;
    x_prev=x_prev;
    y_prev=y_prev;
    //

    while(runner != NULL) {
        data = runner->data;
        x = data->x;
        y = data->y;

        if (x != -1 || y != -1) {
            // Valid end point

            if (x_prev != -1 || y_prev != -1) {
                // Valid start point -> Valid line segment
                usleep(500);
                send_wacom_event(EV_ABS, ABS_X, (int)y);
                send_wacom_event(EV_ABS, ABS_Y, (int)x);
                // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
                send_wacom_event(EV_ABS, ABS_PRESSURE, 3288);
                send_wacom_event(EV_ABS, ABS_DISTANCE, 70);
                send_wacom_event(EV_ABS, ABS_TILT_X, -6100);
                send_wacom_event(EV_ABS, ABS_TILT_Y, 0);
                send_wacom_event(0, 0, 0);

            } else {
                // Invalid start point -> Stoke start (pen up -> pen down)

                send_wacom_event(EV_KEY, BTN_TOOL_PEN, 1);
                // send_wacom_event(EV_KEY, BTN_TOUCH, 0);
                send_wacom_event(EV_ABS, ABS_X, (int)y);
                send_wacom_event(EV_ABS, ABS_Y, (int)x);
                send_wacom_event(EV_ABS, ABS_PRESSURE, 3240);
                send_wacom_event(EV_ABS, ABS_DISTANCE, 10);
                send_wacom_event(0, 0, 0);
            }

        } else {
            //  Invalid end point
            if (x_prev != -1 || y_prev != -1) {
                // Valid start point -> End Segment
                send_wacom_event(EV_ABS, ABS_PRESSURE, 0);
                send_wacom_event(EV_ABS, ABS_DISTANCE, 80);
                send_wacom_event(0, 0, 0);
                usleep(800);
            } else {
                // In-valid start point -> Ignore extra invalid flags
            }

        }

        x_prev = x;
        y_prev = y;
        runner = runner->next;
    }

    send_wacom_event(EV_KEY, BTN_TOOL_PEN, 0);
    send_wacom_event(0, 0, 0);

    return stroke;
}
//////////////////////////////////////////////////////////////////////////////


static void wacom_char(char ascii_value, bool wrap_ok)
{
    int num_strokes = 0;
    int char_width = 0;
    char* current_font = "hershey";

    const gint8* stroke_data = get_font_char(current_font, ascii_value, &num_strokes, &char_width);
    if (!stroke_data)
        return;

    // Interpolate the strokes
    // static std::vector<float> fstrokes; // static to avoid constant allocation
    //fstrokes.clear();
    //GList* fstrokes = NULL;
    //stroke_interp(stroke_data, num_strokes, fstrokes);
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
// Utilities
    GList* stroke_debug (char* tag, GList* stroke) {
        fprintf(stderr,"DEBUG(%s): stroke_debug\n", tag);
        fprintf(stderr,"DEBUG(%s): length of stroke: %d\n",
                tag,
                g_list_length(stroke));

        GList*     runner;
        point_t* data;

        int count = 0;
        runner = stroke;
        while (runner != NULL) {
            data = runner->data;
            count++;
            fprintf(stderr, "DEBUG(%s): %d (%f,%f)\n",
                    tag,
                    count, data->x, data->y);
            runner = runner->next;
        };

        fprintf(stderr,"DEBUG(%s): length of stroke: %d\n",
                tag,
                g_list_length(stroke));
        return stroke;
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
        x=x;
        y=y;

        press_ui_button(6338,7085);

        exit(0);

        for (int j=0; j<1; j++) {

            for (int i=0; i<NDATA; i++) {
                // segment_translate(&sdata[i],x,y);
                segment_write_data(&sdata[i]);
            }
            usleep(100);

        }

        GList* stroke = NULL;

        const char* current_font = "hershey";
        int ascii_value = 0x41;
        int num_strokes;
        int char_width;

        fprintf(stderr,"Write Font Character - %s (%d)\n", current_font, ascii_value);

        const gint8* stroke_data = get_font_char(current_font, ascii_value, &num_strokes, &char_width);
        // stroke_data = stroke_data;

        // exit(0);

        fprintf(stderr,"DEBUG: num_strokes: %d\n", num_strokes);
        // stroke = stroke_debug("pre-load", stroke);
        stroke = stroke_load(stroke, stroke_data, num_strokes);
        // stroke = stroke_debug("load            ", stroke);
        stroke = stroke_scale(stroke, 60, 60);
        stroke = stroke_translate(stroke, 6000, 7000);
        // stroke = stroke_debug("post_scale      ", stroke);
        stroke = stroke_interpolate(stroke);
        // stroke = stroke_debug("post-interpolate", stroke);
        stroke = stroke_write(stroke);
        // stroke = stroke_debug("post-write      ", stroke);

        exit(0);

        wacom_char(ascii_value, wrap_ok);

    }
