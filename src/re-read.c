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

#include <sys/select.h>

#include <sys/epoll.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <ctype.h> // for isprint() etc.
#include <stdbool.h>

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
// Convert host data to Little Endian for Remarkable 2.
segment_t* convert_from_re2 (segment_t* re2_data, segment_t* host_data) {
    host_data->tv_sec  = le32toh(re2_data->tv_sec);
    host_data->tv_usec = le32toh(re2_data->tv_usec);
    host_data->type    = le16toh(re2_data->type);
    host_data->code    = le16toh(re2_data->code);
    host_data->value   = le32toh(re2_data->value);

    return host_data;
}

int segment_decode (int count, segment_t* segment, segment_t* prev_segment) {
    float delta        = 0.0;
    //    bool new_segment = false;

    int print_bytes = false;
    if (print_bytes == true) {
        printf(" %08X", segment->tv_sec);
        printf(" %08X", segment->tv_usec);
        printf(" %04X", segment->type);
        printf(" %04X", segment->code);
        printf(" %08X", segment->value);
        printf("  ");
    }

    int print_decoded = false;
        // New timestamp -> new frame
    if (segment->tv_sec != prev_segment->tv_sec
        || segment->tv_usec != prev_segment->tv_usec) {
        count++;
        // new_segment = true;

        delta = (segment->tv_sec - prev_segment->tv_sec)
            + (segment->tv_usec - prev_segment->tv_usec) / 1000000.0;

        if (prev_segment->tv_sec == 0){
            delta = 0.0;
        }
        if (print_decoded) printf("%4d %9.6f ", count, delta);
    } else {
        if (print_decoded) printf("               ");
    }

    if (print_decoded == true) {
        printf("%d.%06d %5d %5d %10d",
               segment->tv_sec,
               segment->tv_usec,
               segment->type,
               segment->code,
               segment->value);
        printf(" ");
    }

    // Decode Segment
    int print_disassemble = false;
    if (print_disassemble) {
        switch (segment->type) {
        case 0:
            printf("%-9s", "-");
            break;
        case 1:
            printf("S");
            switch (segment->code) {
            case 320:
                printf("p");
                switch (segment->value) {
                case 0:
                    printf("^");
                    break;
                case 1:
                    printf("v");
                    break;
                default:
                    printf("?");
                }
                break;
            case 321:
                printf("e");
                switch (segment->value) {
                case 0:
                    printf("^");
                    break;
                case 1:
                    printf("v");
                    break;
                default:
                    printf("?");
                }
                break;
            default:
                printf(".");
            }
            printf("      ");
            break;
        case 3:
            printf("P");
            switch (segment->code) {

            case 0:
                printf("x");
                break;

            case 1:
                printf("y");
                break;

            case 24:
                printf("p");
                break;

            case 25:
                printf("d");
                break;

            case 26:
                printf("-");
                break;

            case 27:
                printf("|");
                break;
            }
            printf(" %6d", segment->value);
            break;
        default:
            printf("   ");
        }
    }

    int print_c = true;
    if (print_c == true) {
        printf("  {%3d, %6d, %3d, %3d, %6d},",
               segment->tv_sec,
               segment->tv_usec,
               segment->type,
               segment->code,
               segment->value);
        printf(" ");
    }

    return count;
}


//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

    // TODO Change data to be a structure into segment_t.
    // TODO Change reading single bytes, to reading segment_t record.
    int c;
    int data[16];
    segment_t segment;
    segment_t prev_segment;
    int count = 0;

    prev_segment.tv_sec  = 0;
    prev_segment.tv_usec = 0;
    prev_segment.type    = 0;
    prev_segment.code    = 0;
    prev_segment.value   = 0;

    FILE* fp;
    fp = stdin;

    int nsegment = 0;
    int index    = 0;
    while(1) {
        // Read integer and treat all values as integers.
        // Mixing char types causes problems in calculations.
        c = fgetc(fp);
        if( feof(fp) ) {
            break ;
        }

        data[index] = c;

        // Raw data as read.
        // printf("%02X", c);
        // if ((index+1) % 2 == 0) {
        //    printf(" ");
        // }

        index++;
        if (index == sizeof(segment) ) {
            index = 0;
            nsegment++;
            // Little endian data
            segment.tv_sec =
                data[0]
                + data[1] * 0x100
                + data[2] * 0x10000
                + data[3] * 0x1000000;

            segment.tv_usec =
                data[4]
                + data[5] * 0x100
                + data[6] * 0x10000
                + data[7] * 0x1000000;

            segment.type =
                data[8]
                + data[9] * 0x100;

            segment.code =
                data[10]
                + data[11] * 0x100;

            segment.value =
                data[12]
                + data[13] * 0x100
                + data[14] * 0x10000
                + data[15] * 0x1000000;

            count = segment_decode(count, &segment, &prev_segment);
            prev_segment = segment;
            printf(" //%d",nsegment);
            printf("\n");
        }
    }
    //    leave_input_mode();
}
