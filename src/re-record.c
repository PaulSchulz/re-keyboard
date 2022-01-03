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

/*
  Capturing pen stroke data from reMarkable

  ssh -R /dev/input/event1:`pwd`/re-event1-extract root@10.1.1.240
  This does not work as I suspect that drop bear does not support forwarding of named pipes.

 */

#define EVENTS_IN  "re-event1-extract"
#define EVENTS_OUT "re-event1-inject"
#define DATA       "data"
#define PENDATA    DATA"/pendata.dat"

//////////////////////////////////////////////////////////////////////////////
// Command line parsing with GOption
static int hostname = 1;
static int debug = FALSE;

static GOptionEntry entries[] =
{
    {"host", 'h', 0, G_OPTION_ARG_INT, &hostname,
            "IP or name of Remarkable2 to connect to.", "M"},
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

int create_pipes (void) {
    if (mknod(EVENTS_IN, S_IFIFO, 0) < 0) {
        int errnum = errno;
        fprintf(stderr, "Error creating input pipe for events: %s\n", EVENTS_IN);
        fprintf(stderr, "  %s\n", strerror(errnum));
    };
    if (mknod(EVENTS_OUT, S_IFIFO, 0) < 0) {
        int errnum = errno;
        fprintf(stderr, "Error creating output pipe for events: %s\n", EVENTS_OUT);
        fprintf(stderr, "  %s\n", strerror(errnum));
    };

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
struct termios termios_orig;

void enter_input_mode()
{
	//Settings for stdin (source: svgalib):
	int fd = fileno(stdin);
	struct termios zap;

    tcgetattr(fd, &termios_orig);
    zap = termios_orig;
    zap.c_cc[VMIN] = 1;
	zap.c_cc[VTIME] = 0;
    zap.c_lflag &= ~(ICANON | ECHO);
    //zap.c_lflag = 0;
    tcsetattr(fd, TCSANOW, &zap);
}

void leave_input_mode()
{
	//Restore original stdin
	int fd = fileno(stdin);
	ioctl(fd, TCSETA, &termios_orig);
}

char read_key()
{
	int fd = fileno(stdin);

	char c = '\0';
	int e = read(fd, &c, 1);
	if (e == 0) c = '\0';

	return c;
}

//////////////////////////////////////////////////////////////////////////////
/* Call this to change the terminal related to the stream to "raw" state.
 * (Usually you call this with stdin).
 * This means you get all keystrokes, and special keypresses like CTRL-C
 * no longer generate interrupts.
 *
 * You must restore the state before your program exits, or your user will
 * frantically have to figure out how to type 'reset' blind, to get their terminal
 * back to a sane state.
 *
 * The function returns 0 if success, errno error code otherwise.
 */
static int stream_makeraw(FILE *const stream, struct termios *const state)
{
    struct termios  old, raw, actual;
    int             fd;

    if (!stream)
        return errno = EINVAL;

    /* Tell the C library not to buffer any data from/to the stream. */
    if (setvbuf(stream, NULL, _IONBF, 0))
        return errno = EIO;

    /* Write/discard already buffered data in the stream. */
    fflush(stream);

    /* Termios functions use the file descriptor. */
    fd = fileno(stream);
    if (fd == -1)
        return errno = EINVAL;

    /* Discard all unread input and untransmitted output. */
    tcflush(fd, TCIOFLUSH);

    /* Get current terminal settings. */
    if (tcgetattr(fd, &old))
        return errno;

    /* Store state, if requested. */
    if (state)
        *state = old; /* Structures can be assigned! */

    /* New terminal settings are based on current ones. */
    raw = old;

    /* Because the terminal needs to be restored to the original state,
     * you want to ignore CTRL-C (break). */
    raw.c_iflag |= IGNBRK;  /* do ignore break, */
    raw.c_iflag &= ~BRKINT; /* do not generate INT signal at break. */

    /* Make sure we are enabled to receive data. */
    raw.c_cflag |= CREAD;

    /* Do not generate signals from special keypresses. */
    raw.c_lflag &= ~ISIG;

    /* Do not echo characters. */
    raw.c_lflag &= ~ECHO;

    /* Most importantly, disable "canonical" mode. */
    raw.c_lflag &= ~ICANON;

    /* In non-canonical mode, we can set whether getc() returns immediately
     * when there is no data, or whether it waits until there is data.
     * You can even set the wait timeout in tenths of a second.
     * This sets indefinite wait mode. */
    raw.c_cc[VMIN] = 1;  /* Wait until at least one character available, */
    raw.c_cc[VTIME] = 0; /* Wait indefinitely. */

    /* Set the new terminal settings. */
    if (tcsetattr(fd, TCSAFLUSH, &raw))
        return errno;

    /* tcsetattr() is happy even if it did not set *all* settings.
     * We need to verify. */
    if (tcgetattr(fd, &actual)) {
        const int saved_errno = errno;
        /* Try restoring the old settings! */
        tcsetattr(fd, TCSANOW, &old);
        return errno = saved_errno;
    }

    if (actual.c_iflag != raw.c_iflag ||
        actual.c_oflag != raw.c_oflag ||
        actual.c_cflag != raw.c_cflag ||
        actual.c_lflag != raw.c_lflag) {
        /* Try restoring the old settings! */
        tcsetattr(fd, TCSANOW, &old);
        return errno = EIO;
    }

    /* Success! */
    return 0;
}

/* Call this to restore the saved state.
 *
 * The function returns 0 if success, errno error code otherwise.
 */
static int stream_restore (FILE *const stream, const struct termios *const state)
{
    int fd, result;

    if (!stream || !state)
        return errno = EINVAL;

    /* Write/discard all buffered data in the stream. Ignores errors. */
    fflush(stream);

    /* Termios functions use the file descriptor. */
    fd = fileno(stream);
    if (fd == -1)
        return errno = EINVAL;

    /* Discard all unread input and untransmitted output. */
    do {
        result = tcflush(fd, TCIOFLUSH);
    } while (result == -1 && errno == EINTR);

    /* Restore terminal state. */
    do {
        result = tcsetattr(fd, TCSAFLUSH, state);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        return errno;

    /* Success. */
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
void usage (void) {
    char* spacer="----------------------------------------";
    printf("%s\n", spacer); // ----------------------------
    printf("Setup\n");
    printf("  mkfifo %s\n", EVENTS_IN);
    printf("  mkfifo %s\n", EVENTS_OUT);
    printf("  ssh root@10.1.1.240 \"cat /dev/input/event1\" > re-event1-extract\n");
    printf("  cat re-event1-inject | ssh root@10.1.1.240 \"cat > /dev/input/event1\"\n");
    printf("%s\n", spacer); // ----------------------------
}

//////////////////////////////////////////////////////////////////////////////
void info (void) {
    char* spacer="----------------------------------------";
    fprintf(stdout, "%s\n", spacer); // ----------------------------
    fprintf(stdout, "Press\n");
    fprintf(stdout, "  'r' to record\n");
    fprintf(stdout, "  'q' to quit\n");
    fprintf(stdout, "%s\n", spacer); // ----------------------------
}

//////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // Remove buffer on stdout.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Remove buffer on stdin.
    // enter_input_mode();

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

    char* file_in  = EVENTS_IN;
    char* file_out = PENDATA;
    char* file_raw = PENDATA"-raw";

    // Create pipes if required
    // create_pipes();

    printf("Reading from: %s\n", file_in);
    printf("Writing to:   %s\n", file_out);
    printf("%s\n", spacer); // ----------------------------

    // Read data
    FILE *fp;
    FILE *fp_out;
    FILE *fp_raw;
    // char c;

    printf("INFO: Connect pipes.\n");

    // Open files
    if ((fp = fopen(file_in, "r")) == NULL){
        int errnum = errno;
        fprintf(stderr, "Error opening input file: %s\n", file_in);
        fprintf(stderr, "  %s\n", strerror(errnum));
        exit(1);
    };

    if ((fp_out = fopen(file_out, "w")) == NULL){
        int errnum = errno;
        fprintf(stderr, "Error opening input file: %s\n", file_in);
        fprintf(stderr, "  %s\n", strerror(errnum));
        exit(1);
    };

    if ((fp_raw = fopen(file_raw, "w")) == NULL){
        int errnum = errno;
        fprintf(stderr, "Error opening input file: %s\n", file_in);
        fprintf(stderr, "  %s\n", strerror(errnum));
        exit(1);
    };

    printf("INFO: Setup to receive keypress events.\n");

    fputc('\0', fp_out);

    struct termios termios_saved;
    char c;

    /* Make terminal at standard input unbuffered and raw. */
    if (stream_makeraw(stdin, &termios_saved)) {
        fprintf(stderr, "Cannot make standard input raw: %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    info();
    fflush(stdout);

    int record = FALSE;
    int quit   = FALSE;
    int loop = TRUE;

    debug = TRUE;
    do {
        c = getchar(); /* Or c = getc(stdin); */

        if (debug) {
            if (isprint(c))
                fprintf(stdout,
                        "Received character '%c', code %d = 0%03o = 0x%02x\n",
                        c, c, c, c);
            else
                fprintf(stdout,
                        "Received code %d = 0%03o = 0x%02x\n",
                        c, c, c);
            fflush(stdout);
        }

        switch (c) {
        case 'u':
            usage();
            break;
        case 'd':
            debug = TRUE;
            break;
        case 'r':
            record = TRUE;
            loop = FALSE;
            break;
        case 'q':
            quit = TRUE;
            loop = FALSE;
            break;
        default:
            printf(".");
        }

    } while (loop);

    /* Restore terminal to original state. */
    if (stream_restore(stdin, &termios_saved)) {
        fprintf(stderr, "Cannot restore standard input state: %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    fflush(stdout);

    if (quit == TRUE) {
        printf("INFO: Quit\n");
        exit(0);
    }

    segment_u segment;

    if (record == TRUE) {
        printf("INFO: Recording pen strokes to file: %s\n", file_out);
    }

    printf("INFO: Reading pen stroke data.\n");
    int index = 0;
    while(1) {
        c = fgetc(fp);
        if( feof(fp) ) {
            break ;
        }

        fputc(c,fp_raw);

        segment.data[index] = c;

        index++;
        if (index == sizeof(segment.data) ) {
            index = 0;

            switch (segment.segment.type){
            case 0:
                printf("0");
                break;
            case 3:
                printf(">");
                break;
            default:
                printf(".");
            }


            fwrite(segment.data, sizeof(segment.data), 1, fp_out);
        }
    }
    fclose(fp);
    fclose(fp_out);
    fclose(fp_raw);
    //    leave_input_mode();
}
