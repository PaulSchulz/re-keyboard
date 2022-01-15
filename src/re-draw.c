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
}

//////////////////////////////////////////////////////////////////////////////
void segment_write_end (segment_t* data) {
    data->type  = 0;
    data->code  = 0;
    data->value = 0;
    segment_write_data(data);
}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // Remove buffer on stdout.
    setvbuf(stdout, NULL, _IONBF, 0);

    //
    segment_t data;

    // Draw a line
    int x;
    int y;

    x = 14129;
    y = 2800;

    // Pen down
    //segment_set_time(&data);
    //segment_pen_down(&data);

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
