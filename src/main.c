// Re-Keybaord
// Program to emulate a keyboard and fonts
// on the  the Remarkable2 tablet.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <endian.h>

#include <sys/time.h>
#include <glib.h>

#include <sys/stat.h>

#define DELAY 40 // ms between packets

//////////////////////////////////////////////////////////////////////////////
// Command line parsing with GOption
static int hostname = 1;

static GOptionEntry entries[] =
{
    {"host", 'h', 0, G_OPTION_ARG_INT, &hostname,
     "IP or name of Remarkable2 to connect to.", "M"},
    {NULL}
};

//////////////////////////////////////////////////////////////////////////////
// Remarkable Data
typedef struct _segment_t{
    guint32 tv_sec;
    guint32 tv_usec;
    guint16 type;
    guint16 code;
    guint32 value;
} segment_t;

// Convert host data to Little Endian for Remarkable 2.
segment_t* convert_segment (segment_t* host_data, segment_t* re2_data) {
    re2_data->tv_sec  = htole32(host_data->tv_sec);
    re2_data->tv_usec = htole32(host_data->tv_usec);
    re2_data->type    = htole16(host_data->type);
    re2_data->code    = htole16(host_data->code);
    re2_data->value   = htole32(host_data->value);

    return re2_data;
}

segment_t* set_segment_timestamp (segment_t* segment, struct timeval* timestamp){
    struct timeval current_time;

    if (timestamp == NULL) {
        gettimeofday(&current_time, NULL);
    } else {
        current_time.tv_sec = timestamp->tv_sec;
        current_time.tv_usec = timestamp->tv_usec;
    }

    printf("DEBUG: %ld.%ld\n", current_time.tv_sec, current_time.tv_usec);

    segment->tv_sec  = current_time.tv_sec;
    segment->tv_usec = current_time.tv_usec;

    return segment;
}

// Add data segment, and optionally, set the timestamp.
// Need ability to set same timestamp  across multiple data segments.
GList* add_path (GList* path, segment_t* segment, struct timeval* timestamp) {
    segment_t* new_segment = NULL;
    new_segment = malloc(sizeof(segment_t));
    set_segment_timestamp(new_segment,timestamp);

    new_segment->type  = segment->type;
    new_segment->code  = segment->code;
    new_segment->value = segment->value;

    path = g_list_append(path,new_segment);

    return path;
}

//////////////////////////////////////////////////////////////////////////////
// Primitives
//////////////////////////////////////////////////////////////////////////////
// Add 'segment end' code with possible timestamp.
GList* add_path_end (GList* path, struct timeval* timestamp) {
    segment_t segment;
    segment.type  =   0;
    segment.code  =   0;
    segment.value =   0;
    path = add_path(path,&segment,timestamp);
    return path;
}

// Add 'pen down' code with possible timestamp.
GList* add_path_pen_down (GList* path, struct timeval* timestamp) {
    segment_t segment;
    segment.type  =   1;
    segment.code  = 320;
    segment.value =   1;
    path = add_path(path,&segment,timestamp);
    return path;
}

// Add 'pen up' code with possible timestamp.
GList* add_path_pen_up (GList* path, struct timeval* timestamp) {
    segment_t segment;
    segment.type  =   1;
    segment.code  = 320;
    segment.value =   0;
    path = add_path(path,&segment,timestamp);
    return path;
}

// Add 'pen up' code with possible timestamp.
GList* add_path_pressure (GList* path, struct timeval* timestamp) {
    segment_t segment;
    segment.type  =    3;
    segment.code  =   24;
    segment.value = 3472;
    path = add_path(path,&segment,timestamp);
    return path;
}

// Set pen angle
GList* add_path_angle_set (GList* path, struct timeval* timestamp) {
    segment_t segment;
    segment.type  =   3;
    segment.code  =  25;
    segment.value =  56;
    path = add_path(path,&segment,timestamp);

    segment.type  =   3;
    segment.code  =  26;
    segment.value =  4294966296;
    path = add_path(path,&segment,timestamp);

    segment.type  =   3;
    segment.code  =  27;
    segment.value =  4294964996;
    path = add_path(path,&segment,timestamp);

    return path;
}

// Add absolute position x,y with possible timestamp
GList* add_path_to (GList* path, guint32 x, guint32 y, struct timeval* timestamp) {
    segment_t segment;

    // timestamp may be
    segment.type  =   3;
    segment.code  =   0;
    segment.value =   y;
    path = add_path(path,&segment,timestamp);

    segment.type  =   3;
    segment.code  =   1;
    segment.value =   x;
    path = add_path(path,&segment,timestamp);

    printf("DEBUG: add_path_to  %ld.%ld\n", timestamp->tv_sec, timestamp->tv_usec);

    return path;
}

//////////////////////////////////////////////////////////////////////////////
void path_write (GList* path){
    while(path != NULL){
        printf("%d.%d [%08X %08X]\n",
               ((segment_t*)path->data)->tv_sec,
               ((segment_t*)path->data)->tv_usec,
               ((segment_t*)path->data)->tv_sec,
               ((segment_t*)path->data)->tv_usec
            );
        path = path->next;
    }
}

void path_write_to_file (GList* path){
    FILE *file = fopen("path-data","wb");
    segment_t segment;

    while(path != NULL){
        convert_segment((segment_t*)(path->data),&segment);
        //segment.tv_sec  = ((segment_t*)(path->data))->tv_sec;
        //segment.tv_usec = ((segment_t*)(path->data))->tv_usec;
        //segment.type    = ((segment_t*)(path->data))->type;
        //segment.code    = ((segment_t*)(path->data))->code;
        segment.value   = ((segment_t*)(path->data))->value;
        fwrite(&segment, 16, 1, file);
        path = path->next;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Tests
void path_delay(void) {
    usleep(40000);
}

struct timeval* new_timestamp (struct timeval* timestamp) {
    gettimeofday(timestamp, NULL);
    return timestamp;
}

GList* add_path_test (GList* path) {
    struct timeval timestamp;

    new_timestamp(&timestamp);
    path = add_path_pen_down(path,&timestamp);
    path = add_path_to(path,3453,18433,&timestamp);
    path = add_path_angle_set(path,&timestamp);
    path = add_path_pressure(path,&timestamp);
    path = add_path_end(path,&timestamp);

    for(int i=0; i<1000; i++){
        path_delay();
        new_timestamp(&timestamp);
        path = add_path_to(path, 3454+i, 18434, &timestamp);
        path = add_path_end(path, &timestamp);

        printf("%i\n",i);
    }

    path_delay();
    new_timestamp(&timestamp);
    path = add_path_pen_up(path, &timestamp);
    path = add_path_end(path, &timestamp);

    path_write(path);
    path_write_to_file(path);
    return path;
}


//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // Parse command line
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- Virtual keyboard function for Remarkable2");
    g_option_context_add_main_entries (context, entries, NULL);
    //g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    //g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    /* printf function displays the content that is
     * passed between the double quotes.
     */

    char* spacer="----------------------------------------";
    printf("%s\n", spacer); // ----------------------------
    printf("Remarkable2 (Network) Keyboard\n");
    printf("%s\n", spacer); // ----------------------------
    printf("  mkfifo re-event1-extract\n");
    printf("  mkfifo re-event1-inject\n");
    printf("  ssh root@10.1.1.240 \"cat /dev/input/event1\" > re-event1-extract\n");
    printf("  cat re-event1-inject | ssh root@10.1.1.240 \"cat > re-event1-extract\"\n");
    printf("%s\n", spacer); // ----------------------------
    char* file_in  = "re-event1-extract";
    char* file_out = "data-out";

    printf("Reading from: %s\n", file_in);
    printf("Writing to:   %s\n", file_out);
    printf("%s\n", spacer); // ----------------------------


// Open file to read
    int fd = open("re-event1-extract", O_RDONLY);
    if (fd == 0 ) {
        printf("File un-successfully opened\n");
        exit(0);
    };

    printf("File successfully opened\n");
    int buff[100];
    int readsize;

    readsize = read(fd,buff,100);
    while(readsize > 0){
        printf(".");
    }

// test
//GList* path = NULL;
//path = add_path_test (path);

        return 0;
}
