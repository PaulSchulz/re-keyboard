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
    //    putchar((data->value / 0x1000000) & 0xFF);
    putchar((data->value>>(8*3)) & 0xFF);

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

segment_t sdata[7] =
{
    {0, 0, 1, 320,         1},
    {0, 0, 3,   0,     18656},
    {0, 0, 3,   1,     15054},
    {0, 0, 3,  25,        61},
    {0, 0, 3,  26, 284161696},
    {0, 0, 3,  27, 284162096},
    {0, 0, 0,   0,         0},
};

GList* add_segment_to_path (GList* path, segment_t* segment) {
    return path;
}

// Calculate the metics (extents) for a path.
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

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    // Remove buffer on stdout.
    setvbuf(stdout, NULL, _IONBF, 0);

    //
    segment_t data;

    // Draw a line
    int x;
    int y;

    x = 14129;
    y = 2800;


    for (int i=0; i<7; i++) {
        segment_write_data(&sdata[i]);
    }

    exit(0);

    // Pen point
    segment_set_time(&data);

    data.type = 1;
    data.code = 320;
    data.value = 1;
    segment_write_data(&data);
    data.type = 3;
    data.code = 0;
    data.value = x;
    segment_write_data(&data);
    data.type = 3;
    data.code = 1;
    data.value = y;
    segment_write_data(&data);

    data.type = 3;
    data.code = 25;
    data.value = 70;
    segment_write_data(&data);
    data.type = 3;
    data.code = 26;
    data.value = 284161296;
    segment_write_data(&data);
    data.type = 3;
    data.code = 27;
    data.value = 284161296;
    segment_write_data(&data);

    segment_write_end(&data);
    usleep(1600);

    for(int i=0; i<1000; i++) {
        x += 5;

        segment_set_time(&data);
        data.type = 3;
        data.code = 0;
        data.value = x;
        segment_write_data(&data);
        data.type = 3;
        data.code = 1;
        data.value = y;
        segment_write_data(&data);
        data.type = 3;
        data.code = 25;
        data.value = 10;
        segment_write_data(&data);

        segment_write_end(&data);
        usleep(1600);

    }

    segment_set_time(&data);
        data.type = 1;
        data.code = 320;
        data.value = 0;
        segment_write_data(&data);
        segment_write_end(&data);

        // Pen up
        //segment_set_time(&data);
        //segment_pen_up(&data);
        //segment_write_data(&data);
        usleep(1600);
    }
