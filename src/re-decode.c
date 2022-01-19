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

#include <ctype.h>
// for isprint() etc.

//////////////////////////////////////////////////////////////////////////////
// Command line parsing with GOption
static int debug = TRUE;

static GOptionEntry entries[] =
{
    {NULL}
};

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

// Convert host data to Little Endian for Remarkable 2.
segment_t* convert_to_re2 (segment_t* host_data, segment_t* re2_data) {
    re2_data->tv_sec  = htole32(host_data->tv_sec);
    re2_data->tv_usec = htole32(host_data->tv_usec);
    re2_data->type    = htole16(host_data->type);
    re2_data->code    = htole16(host_data->code);
    re2_data->value   = htole32(host_data->value);

    return re2_data;
}
//////////////////////////////////////////////////////////////////////////////
segment_t* relative_time (segment_t* segment_p, segment_t first_segment) {
    segment_p->tv_sec = segment_p->tv_sec - first_segment.tv_sec;

    return segment_p;
}

//////////////////////////////////////////////////////////////////////////////
void print_segment_formatted (segment_t segment){

    printf("%d.%06d %3d/%3d",
           segment.tv_sec,
           segment.tv_usec,
           segment.code,
           segment.type
        );
    printf(" ");

    switch (segment.code) {
    case 0:
        printf("E\n");
        break;
    case 3:
        printf("P");
        switch (segment.type) {
        case 0:
            printf("y  ");
            break;
        case 1:
            printf("x  ");
            break;
        case 24:
            printf("p  ");
            break;
        case 25:
            printf("d  ");
            break;
        case 26:
            printf("tx ");
            break;
        case 27:
            printf("ty ");
            break;
        default:
            printf("-  ");
        }
        printf("%d",segment.value);
        break;
    default:
        printf(".");
    }
}

//////////////////////////////////////////////////////////////////////////////
void usage (void) {
    char* spacer="----------------------------------------";
    printf("%s\n", spacer); // ----------------------------
}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // Remove buffer on stdout.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Parse command line
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- Record pen strokes from Remarkable2");
    g_option_context_add_main_entries (context, entries, NULL);
    //g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    //g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    char* spacer="----------------------------------------";
    printf("%s\n", spacer); // ----------------------------
    printf("Remarkable2 (Network) Recorder\n");
    printf("%s\n", spacer); // ----------------------------

    usage();

    if (debug == TRUE) {
        printf("Reading from stdin\n");
        printf("Writing to stdout\n");
        printf("Segment size: %ld bytes\n", sizeof(segment_t));
        printf("%s\n", spacer); // ----------------------------
    }

    int count = 0;
    int c; // Returned value from
    char segment_buffer[sizeof(segment_t)];
    segment_t segment;

    FILE* stream = stdin;

    //segment_t first_segment;
    // first_segment.tv_sec = 0;
    // first_segment.tv_usec = 0;

    int loop  = TRUE;
    int index = 0;
    while(loop) {
        if (index == 0) {
            printf("%06d: ",count);
        }

        c = fgetc(stream);
        if( feof(stream) ) {
            break ;
        }

        segment_buffer[index] = (char)c;

        // printf(".");
        printf("%02x", c);
        switch (index) {
        case 3:
            printf(" | ");
            break;
        case 7:
            printf(" | ");
            break;
        case 9:
            printf(" | ");
            break;
        case 11:
            printf(" | ");
            break;
        case 16:
            printf(" | ");
            break;
        default:
            printf(" ");
        }

        index++;
        if (index == sizeof(segment) ) {
            index = 0;

            // Decode buffer
            segment.tv_sec =
                (segment_buffer[0] & 0xFF)
                + (segment_buffer[1] & 0xFF) * 0x100
                + (segment_buffer[2] & 0xFF) * 0x10000
                + (segment_buffer[3] & 0xFF) * 0x1000000;

            segment.tv_usec =
                (segment_buffer[4] & 0xFF)
                + (segment_buffer[5] & 0xFF) * 0x100
                + (segment_buffer[6] & 0xFF) * 0x10000
                + (segment_buffer[7] & 0xFF) * 0x1000000;

            segment.code =
                (segment_buffer[8] & 0xFF)
                + (segment_buffer[9] & 0xFF) * 0x100;

            segment.type =
                (segment_buffer[10] & 0xFF)
                + (segment_buffer[11] & 0xFF) * 0x100;

            segment.value =
                (segment_buffer[12] & 0xFF)
                + (segment_buffer[13] & 0xFF) * 0x100
                + (segment_buffer[14] & 0xFF) * 0x10000
                + (segment_buffer[15] & 0xFF) * 0x1000000;

            //if (count == 0) {
            //    first_segment = segment;
            //}
            //relative_time(&segment,first_segment);

            printf(" -  ");
            print_segment_formatted(segment);

            printf("\n");
            count++;
        }
    }
    printf("\n");
}
