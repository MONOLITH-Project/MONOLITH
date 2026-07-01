/*
 * Copyright (c) 2011-2018 K. Lange.  All rights reserved.
 *
 * Developed by:            K. Lange
 *                          http://github.com/klange/nyancat
 *                          http://nyancat.dakko.us
 *
 * 40-column support by:    Peter Hazenberg
 *                          http://github.com/Peetz0r/nyancat
 *                          http://peter.haas-en-berg.nl
 *
 * Build tools unified by:  Aaron Peschel
 *                          https://github.com/apeschel
 *
 * For a complete listing of contributors, please see the git commit history.
 *
 * This is a simple telnet server / standalone application which renders the
 * classic Nyan Cat (or "poptart cat") to your terminal.
 *
 * It makes use of various ANSI escape sequences to render color, or in the case
 * of a VT220, simply dumps text to the screen.
 *
 * For more information, please see:
 *
 *     http://nyancat.dakko.us
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimers in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the Association for Computing Machinery, K.
 *      Lange, nor the names of its contributors may be used to endorse
 *      or promote products derived from this Software without specific prior
 *      written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE 1
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define __BSD_VISIBLE 1
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <resource.h>
#include <term.h>

#include "telnet.h"

/*
 * The animation frames are stored separately in
 * this header so they don't clutter the core source
 */
#include "animation.h"

#define monolith_sleep_ms usleep

#define no_argument 0
#define required_argument 1

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

char *optarg;
static int optind = 1;
static char output_buffer[TERM_MAX_PAYLOAD];
static uint32_t output_buffer_len = 0;
extern int terminal_width;
extern int terminal_height;
static void write_bytes(const char *text, uint32_t length);
static void flush_output(void);
typedef uint64_t time_t;
typedef int jmp_buf[1];
struct winsize { int ws_col; int ws_row; };
#define SIGINT 2
#define SIGALRM 14
#define SIGPIPE 13
#define SIGWINCH 28
#define TIOCGWINSZ 0x5413
#define stdin ((void *) 0)

static int signal(int sig, void (*handler)(int)) { (void) sig; (void) handler; return 0; }
static int alarm(int seconds) { (void) seconds; return 0; }
static int setjmp(jmp_buf env) { (void) env; return 1; }
static void longjmp(jmp_buf env, int value) { (void) env; (void) value; }
static int feof(void *stream) { (void) stream; return 1; }
static int getchar(void) { return -1; }
static int ioctl(int fd, int request, struct winsize *w) {
	(void) fd;
	(void) request;
	if (w != NULL) {
		w->ws_col = terminal_width;
		w->ws_row = terminal_height;
	}
	return 0;
}
static char *strndup(const char *s, size_t n) { (void) s; (void) n; return NULL; }
static char *getenv(const char *name) {
	static char term[] = "truecolor";
	(void) name;
	return term;
}
static int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; }
static char *strstr(const char *haystack, const char *needle) {
	size_t needle_len = strlen(needle);
	if (needle_len == 0) return (char *) haystack;
	for (; *haystack; haystack++) {
		if (strncmp(haystack, needle, needle_len) == 0) return (char *) haystack;
	}
	return NULL;
}
static int time(time_t *out) {
	time_t seconds = get_ticks() / 1000;
	if (out != NULL) *out = seconds;
	return (int) seconds;
}
static double difftime(time_t end, time_t start) { return (double) (end - start); }
static int atoi(const char *text) {
	int sign = 1;
	int value = 0;
	if (text == NULL) return 0;
	if (*text == '-') {
		sign = -1;
		text++;
	}
	while (*text >= '0' && *text <= '9') {
		value = value * 10 + (*text - '0');
		text++;
	}
	return value * sign;
}
static void nyan_usleep(uint64_t usec) {
	uint64_t ms = (usec + 999) / 1000;
	monolith_sleep_ms(ms == 0 ? 1 : ms);
}
#undef usleep
#define usleep(usec) nyan_usleep(usec)

static char *format_unsigned(char *out, unsigned long value) {
	char tmp[32];
	int len = 0;
	if (value == 0) {
		*out++ = '0';
		return out;
	}
	while (value > 0) {
		tmp[len++] = (char) ('0' + (value % 10));
		value /= 10;
	}
	while (len > 0) *out++ = tmp[--len];
	return out;
}

static int terminal_vprintf(const char *format, va_list args) {
	char buffer[2048];
	char *out = buffer;
	const char *p = format;
	while (*p != '\0' && (size_t) (out - buffer) < sizeof(buffer) - 1) {
		if (*p != '%') {
			*out++ = *p++;
			continue;
		}
		p++;
		while (*p == '0' || *p == '.' || (*p >= '0' && *p <= '9')) p++;
		switch (*p++) {
		case 's': {
			const char *s = va_arg(args, const char *);
			if (s == NULL) s = "(null)";
			while (*s != '\0' && (size_t) (out - buffer) < sizeof(buffer) - 1) *out++ = *s++;
			break;
		}
		case 'd': {
			int value = va_arg(args, int);
			if (value < 0) {
				*out++ = '-';
				value = -value;
			}
			out = format_unsigned(out, (unsigned int) value);
			break;
		}
		case 'u':
			out = format_unsigned(out, va_arg(args, unsigned int));
			break;
		case 'f': {
			double value = va_arg(args, double);
			if (value < 0) {
				*out++ = '-';
				value = -value;
			}
			out = format_unsigned(out, (unsigned long) (value + 0.5));
			break;
		}
		case 'c':
			*out++ = (char) va_arg(args, int);
			break;
		case '%':
			*out++ = '%';
			break;
		default:
			break;
		}
	}
	*out = '\0';
	write_bytes(buffer, (uint32_t) (out - buffer));
	return (int) (out - buffer);
}

static void flush_output(void) {
	if (output_buffer_len == 0) return;
	term_write_command(TERM_RD_TIN, TERM_COMMAND_WRITE_VT100, output_buffer, output_buffer_len);
	output_buffer_len = 0;
}

static void write_bytes(const char *text, uint32_t length) {
	while (length > 0) {
		uint32_t available = TERM_MAX_PAYLOAD - output_buffer_len;
		uint32_t chunk = length < available ? length : available;
		memcpy(output_buffer + output_buffer_len, text, chunk);
		output_buffer_len += chunk;
		text += chunk;
		length -= chunk;
		if (output_buffer_len == TERM_MAX_PAYLOAD) flush_output();
	}
}

static int terminal_printf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	int written = terminal_vprintf(format, args);
	va_end(args);
	return written;
}

static void terminal_putc(char c) {
	write_bytes(&c, 1);
}

static int arg_matches_long(const char *arg, const char *name) {
	size_t len = strlen(name);
	return strncmp(arg, "--", 2) == 0 && strncmp(arg + 2, name, len) == 0
		&& (arg[len + 2] == '\0' || arg[len + 2] == '=');
}

static int short_option_requires_arg(const char *short_opts, int option) {
	for (const char *p = short_opts; p != NULL && *p != '\0'; p++) {
		if (*p == option)
			return p[1] == ':';
	}
	return 0;
}

static int getopt_long(int argc, char **argv, const char *short_opts, const struct option *long_opts, int *index) {
	if (optind >= argc) return -1;

	char *arg = argv[optind++];
	optarg = NULL;
	if (arg[0] != '-') return '?';
	if (arg[1] != '-') {
		int c = arg[1];
		if (short_option_requires_arg(short_opts, c)) {
			if (arg[2] != '\0') optarg = &arg[2];
			else if (optind < argc) optarg = argv[optind++];
		}
		return c;
	}

	for (int i = 0; long_opts[i].name != NULL; i++) {
		if (!arg_matches_long(arg, long_opts[i].name)) continue;
		if (index != NULL) *index = i;
		char *eq = arg;
		while (*eq != '\0' && *eq != '=') eq++;
		if (*eq == '=') optarg = eq + 1;
		else if (long_opts[i].has_arg == required_argument && optind < argc) optarg = argv[optind++];
		return long_opts[i].flag == NULL ? long_opts[i].val : 0;
	}
	return '?';
}

static void update_terminal_size(term_event_type_t expected) {
	char payload[TERM_MAX_PAYLOAD];
	for (int attempt = 0; attempt < 100; attempt++) {
		term_event_t event = {0};
		int result = term_read_event(TERM_RD_TOUT, &event, payload, sizeof(payload));
		if (result < 0) return;
		if (result == 0) {
			if (expected == TERM_EVENT_WINDOW_RESIZED) return;
			usleep(10);
			continue;
		}
		if (event.type != expected || event.length < sizeof(term_dimensions_t)) continue;
		term_dimensions_t *dimensions = (term_dimensions_t *) payload;
		if (dimensions->cols > 0) terminal_width = dimensions->cols;
		if (dimensions->rows > 0) terminal_height = dimensions->rows;
		return;
	}
}

#define printf terminal_printf
#define putc(c, stream) terminal_putc((char) (c))
#define fflush(stream) flush_output()
#define stdout ((void *) 0)

/*
 * Color palette to use for final output
 * Specifically, this should be either control sequences
 * or raw characters (ie, for vt220 mode)
 */
const char * colors[256] = {NULL};

/*
 * For most modes, we output spaces, but for some
 * we will use block characters (or even nothing)
 */
const char * output = "  ";

/*
 * Are we currently in telnet mode?
 */
int telnet = 0;

/*
 * Whether or not to show the counter
 */
int show_counter = 1;

/*
 * Number of frames to show before quitting
 * or 0 to repeat forever (default)
 */
unsigned int frame_count = 0;

/*
 * Clear the screen between frames (as opposed to resetting
 * the cursor position)
 */
int clear_screen = 1;

/*
 * Force-set the terminal title.
 */
int set_title = 1;

/*
 * Environment to use for setjmp/longjmp
 * when breaking out of options handler
 */
jmp_buf environment;


/*
 * I refuse to include libm to keep this low
 * on external dependencies.
 *
 * Count the number of digits in a number for
 * use with string output.
 */
int digits(int val) {
	int d = 1, c;
	if (val >= 0) for (c = 10; c <= val; c *= 10) d++;
	else for (c = -10 ; c >= val; c *= 10) d++;
	return (c < 0) ? ++d : d;
}

/*
 * These values crop the animation, as we have a full 64x64 stored,
 * but we only want to display 40x24 (double width).
 */
int min_row = -1;
int max_row = -1;
int min_col = -1;
int max_col = -1;

/*
 * Actual width/height of terminal.
 */
int terminal_width = 80;
int terminal_height = 24;

/*
 * Flags to keep track of whether width/height were automatically set.
 */
char using_automatic_width = 0;
char using_automatic_height = 0;

/*
 * Print escape sequences to return cursor to visible mode
 * and exit the application.
 */
void finish() {
	if (clear_screen) {
		printf("\033[?25h\033[0m\033[H\033[2J");
	} else {
		printf("\033[0m\n");
	}
	fflush(stdout);
	exit(0);
}

/*
 * In the standalone mode, we want to handle an interrupt signal
 * (^C) so that we can restore the cursor and clear the terminal.
 */
void SIGINT_handler(int sig){
	(void)sig;
	finish();
}

/*
 * Handle the alarm which breaks us off of options
 * handling if we didn't receive a terminal
 */
void SIGALRM_handler(int sig) {
	(void)sig;
	alarm(0);
	longjmp(environment, 1);
	/* Unreachable */
}

/*
 * Handle the loss of stdout, as would be the case when
 * in telnet mode and the client disconnects
 */
void SIGPIPE_handler(int sig) {
	(void)sig;
	finish();
}

void SIGWINCH_handler(int sig) {
	(void)sig;
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	terminal_width = w.ws_col;
	terminal_height = w.ws_row;

	if (using_automatic_width) {
		min_col = (FRAME_WIDTH - terminal_width/2) / 2;
		max_col = (FRAME_WIDTH + terminal_width/2) / 2;
	}

	if (using_automatic_height) {
		min_row = (FRAME_HEIGHT - (terminal_height-1)) / 2;
		max_row = (FRAME_HEIGHT + (terminal_height-1)) / 2;
	}

	signal(SIGWINCH, SIGWINCH_handler);
}

/*
 * Telnet requires us to send a specific sequence
 * for a line break (\r\000\n), so let's make it happy.
 */
void newline(int n) {
	int i = 0;
	for (i = 0; i < n; ++i) {
		/* We will send `n` linefeeds to the client */
		if (telnet) {
			/* Send the telnet newline sequence */
			putc('\r', stdout);
			putc(0, stdout);
			putc('\n', stdout);
		} else {
			/* Send a regular line feed */
			putc('\n', stdout);
		}
	}
}

/*
 * These are the options we want to use as
 * a telnet server. These are set in set_options()
 */
unsigned char telnet_options[256] = { 0 };
unsigned char telnet_willack[256] = { 0 };

/*
 * These are the values we have set or
 * agreed to during our handshake.
 * These are set in send_command(...)
 */
unsigned char telnet_do_set[256]  = { 0 };
unsigned char telnet_will_set[256]= { 0 };

/*
 * Set the default options for the telnet server.
 */
void set_options() {
	/* We will not echo input */
	telnet_options[ECHO] = WONT;
	/* We will set graphics modes */
	telnet_options[SGA]  = WILL;
	/* We will not set new environments */
	telnet_options[NEW_ENVIRON] = WONT;

	/* The client should echo its own input */
	telnet_willack[ECHO]  = DO;
	/* The client can set a graphics mode */
	telnet_willack[SGA]   = DO;
	/* The client should not change, but it should tell us its window size */
	telnet_willack[NAWS]  = DO;
	/* The client should tell us its terminal type (very important) */
	telnet_willack[TTYPE] = DO;
	/* No linemode */
	telnet_willack[LINEMODE] = DONT;
	/* And the client can set a new environment */
	telnet_willack[NEW_ENVIRON] = DO;
}

/*
 * Send a command (cmd) to the telnet client
 * Also does special handling for DO/DONT/WILL/WONT
 */
void send_command(int cmd, int opt) {
	/* Send a command to the telnet client */
	if (cmd == DO || cmd == DONT) {
		/* DO commands say what the client should do. */
		if (((cmd == DO) && (telnet_do_set[opt] != DO)) ||
			((cmd == DONT) && (telnet_do_set[opt] != DONT))) {
			/* And we only send them if there is a disagreement */
			telnet_do_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else if (cmd == WILL || cmd == WONT) {
		/* Similarly, WILL commands say what the server will do. */
		if (((cmd == WILL) && (telnet_will_set[opt] != WILL)) ||
			((cmd == WONT) && (telnet_will_set[opt] != WONT))) {
			/* And we only send them during disagreements */
			telnet_will_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else {
		/* Other commands are sent raw */
		printf("%c%c", IAC, cmd);
	}
}

/*
 * Print the usage / help text describing options
 */
void usage(char * argv[]) {
	printf(
			"Terminal Nyancat\n"
			"\n"
			"usage: %s [-hitn] [-f \033[3mframes\033[0m]\n"
			"\n"
			" -i --intro      \033[3mShow the introduction / about information at startup.\033[0m\n"
			" -t --telnet     \033[3mTelnet mode.\033[0m\n"
			" -n --no-counter \033[3mDo not display the timer\033[0m\n"
			" -s --no-title   \033[3mDo not set the titlebar text\033[0m\n"
			" -e --no-clear   \033[3mDo not clear the display between frames\033[0m\n"
			" -d --delay      \033[3mDelay image rendering by anywhere between 10ms and 1000ms\n"
			" -f --frames     \033[3mDisplay the requested number of frames, then quit\033[0m\n"
			" -r --min-rows   \033[3mCrop the animation from the top\033[0m\n"
			" -R --max-rows   \033[3mCrop the animation from the bottom\033[0m\n"
			" -c --min-cols   \033[3mCrop the animation from the left\033[0m\n"
			" -C --max-cols   \033[3mCrop the animation from the right\033[0m\n"
			" -W --width      \033[3mCrop the animation to the given width\033[0m\n"
			" -H --height     \033[3mCrop the animation to the given height\033[0m\n"
			" -h --help       \033[3mShow this help message.\033[0m\n",
			argv[0]);
	fflush(stdout);
}

int main(int argc, char ** argv) {

	char *term = NULL;
	unsigned int k;
	int ttype;
	uint32_t option = 0, done = 0, sb_mode = 0;
	/* Various pieces for the telnet communication */
	unsigned char  sb[1024] = {0};
	unsigned short sb_len   = 0;

	/* Whether or not to show the MOTD intro */
	char show_intro = 0;
	char skip_intro = 0;

	/* Long option names */
	static struct option long_opts[] = {
		{"help",       no_argument,       0, 'h'},
		{"telnet",     no_argument,       0, 't'},
		{"intro",      no_argument,       0, 'i'},
		{"skip-intro", no_argument,       0, 'I'},
		{"no-counter", no_argument,       0, 'n'},
		{"no-title",   no_argument,       0, 's'},
		{"no-clear",   no_argument,       0, 'e'},
		{"delay",      required_argument, 0, 'd'},
		{"frames",     required_argument, 0, 'f'},
		{"min-rows",   required_argument, 0, 'r'},
		{"max-rows",   required_argument, 0, 'R'},
		{"min-cols",   required_argument, 0, 'c'},
		{"max-cols",   required_argument, 0, 'C'},
		{"width",      required_argument, 0, 'W'},
		{"height",     required_argument, 0, 'H'},
		{0,0,0,0}
	};

	/* Time delay in milliseconds */
	int delay_ms = 90; // Default to original value

	/* Process arguments */
	int index = 0, c;
	while ((c = getopt_long(argc, argv, "eshiItnd:f:r:R:c:C:W:H:", long_opts, &index)) != -1) {
		if (!c) {
			if (long_opts[index].flag == 0) {
				c = long_opts[index].val;
			}
		}
		switch (c) {
			case 'e':
				clear_screen = 0;
				break;
			case 's':
				set_title = 0;
				break;
			case 'i': /* Show introduction */
				show_intro = 1;
				break;
			case 'I':
				skip_intro = 1;
				break;
			case 't': /* Expect telnet bits */
				printf("nyancat: telnet mode is not available on MONOLITH\n");
				fflush(stdout);
				exit(1);
				break;
			case 'h': /* Show help and exit */
				usage(argv);
				exit(0);
				break;
			case 'n':
				show_counter = 0;
				break;
			case 'd':
				if (10 <= atoi(optarg) && atoi(optarg) <= 1000)
					delay_ms = atoi(optarg);
				break;
			case 'f':
				frame_count = atoi(optarg);
				break;
			case 'r':
				min_row = atoi(optarg);
				break;
			case 'R':
				max_row = atoi(optarg);
				break;
			case 'c':
				min_col = atoi(optarg);
				break;
			case 'C':
				max_col = atoi(optarg);
				break;
			case 'W':
				min_col = (FRAME_WIDTH - atoi(optarg)) / 2;
				max_col = (FRAME_WIDTH + atoi(optarg)) / 2;
				break;
			case 'H':
				min_row = (FRAME_HEIGHT - atoi(optarg)) / 2;
				max_row = (FRAME_HEIGHT + atoi(optarg)) / 2;
				break;
			default:
				break;
		}
	}

	if (telnet) {
		/* Telnet mode */

		/* show_intro is implied unless skip_intro was set */
		show_intro = (skip_intro == 0) ? 1 : 0;

		/* Set the default options */
		set_options();

		/* Let the client know what we're using */
		for (option = 0; option < 256; option++) {
			if (telnet_options[option]) {
				send_command(telnet_options[option], option);
				fflush(stdout);
			}
		}
		for (option = 0; option < 256; option++) {
			if (telnet_willack[option]) {
				send_command(telnet_willack[option], option);
				fflush(stdout);
			}
		}

		/* Set the alarm handler to execute the longjmp */
		signal(SIGALRM, SIGALRM_handler);

		/* Negotiate options */
		if (!setjmp(environment)) {
			/* We will stop handling options after one second */
			alarm(1);

			/* Let's do this */
			while (!feof(stdin) && done < 2) {
				/* Get either IAC (start command) or a regular character (break, unless in SB mode) */
				unsigned char i = getchar();
				unsigned char opt = 0;
				if (i == IAC) {
					/* If IAC, get the command */
					i = getchar();
					switch (i) {
						case SE:
							/* End of extended option mode */
							sb_mode = 0;
							if (sb[0] == TTYPE) {
								/* This was a response to the TTYPE command, meaning
								 * that this should be a terminal type */
								alarm(2);
								term = strndup((char *)&sb[2], sizeof(sb)-2);
								done++;
							}
							else if (sb[0] == NAWS) {
								/* This was a response to the NAWS command, meaning
								 * that this should be a window size */
								alarm(2);
								terminal_width = (sb[1] << 8) | sb[2];
								terminal_height = (sb[3] << 8) | sb[4];
								done++;
							}
							break;
						case NOP:
							/* No Op */
							send_command(NOP, 0);
							fflush(stdout);
							break;
						case WILL:
						case WONT:
							/* Will / Won't Negotiation */
							opt = getchar();
							if (!telnet_willack[opt]) {
								/* We default to WONT */
								telnet_willack[opt] = WONT;
							}
							send_command(telnet_willack[opt], opt);
							fflush(stdout);
							if ((i == WILL) && (opt == TTYPE)) {
								/* WILL TTYPE? Great, let's do that now! */
								printf("%c%c%c%c%c%c", IAC, SB, TTYPE, SEND, IAC, SE);
								fflush(stdout);
							}
							break;
						case DO:
						case DONT:
							/* Do / Don't Negotiation */
							opt = getchar();
							if (!telnet_options[opt]) {
								/* We default to DONT */
								telnet_options[opt] = DONT;
							}
							send_command(telnet_options[opt], opt);
							fflush(stdout);
							break;
						case SB:
							/* Begin Extended Option Mode */
							sb_mode = 1;
							sb_len  = 0;
							memset(sb, 0, sizeof(sb));
							break;
						case IAC: 
							/* IAC IAC? That's probably not right. */
							done = 2;
							break;
						default:
							break;
					}
				} else if (sb_mode) {
					/* Extended Option Mode -> Accept character */
					if (sb_len < sizeof(sb) - 1) {
						/* Append this character to the SB string,
						 * but only if it doesn't put us over
						 * our limit; honestly, we shouldn't hit
						 * the limit, as we're only collecting characters
						 * for a terminal type or window size, but better safe than
						 * sorry (and vulnerable).
						 */
						sb[sb_len] = i;
						sb_len++;
					}
				}
			}
		}
		alarm(0);
	} else {
		/* We are running standalone, retrieve the
		 * terminal type from the environment. */
		term = getenv("TERM");

		/* Also get the number of columns */
		term_write_command(TERM_RD_TIN, TERM_COMMAND_GET_TERM_INFO, NULL, 0);
		update_terminal_size(TERM_EVENT_WINDOW_INFO);
	}

	/* Default ttype */
	ttype = 2;

	if (term) {
		/* Convert the entire terminal string to lower case */
		for (k = 0; k < strlen(term); ++k) {
			term[k] = tolower(term[k]);
		}

		/* Do our terminal detection */
		if (strstr(term, "xterm")) {
			ttype = 1; /* 256-color, spaces */
		} else if (strstr(term, "toaru")) {
			ttype = 1; /* emulates xterm */
		} else if (strstr(term, "linux")) {
			ttype = 3; /* Spaces and blink attribute */
		} else if (strstr(term, "vtnt")) {
			ttype = 5; /* Extended ASCII fallback == Windows */
		} else if (strstr(term, "cygwin")) {
			ttype = 5; /* Extended ASCII fallback == Windows */
		} else if (strstr(term, "vt220")) {
			ttype = 6; /* No color support */
		} else if (strstr(term, "fallback")) {
			ttype = 4; /* Unicode fallback */
		} else if (strstr(term, "rxvt-256color")) {
			ttype = 1; /* xterm 256-color compatible */
		} else if (strstr(term, "rxvt")) {
			ttype = 3; /* Accepts LINUX mode */
		} else if (strstr(term, "vt100") && terminal_width == 40) {
			ttype = 7; /* No color support, only 40 columns */
		} else if (!strncmp(term, "st", 2)) {
			ttype = 1; /* suckless simple terminal is xterm-256color-compatible */
		} else if (!strncmp(term, "truecolor", 9)) {
			ttype = 8;
		}
	}

	int always_escape = 0; /* Used for text mode */

	/* Accept ^C -> restore cursor */
	signal(SIGINT, SIGINT_handler);

	/* Handle loss of stdout */
	signal(SIGPIPE, SIGPIPE_handler);

	/* Handle window changes */
	if (!telnet) {
		signal(SIGWINCH, SIGWINCH_handler);
	}

	switch (ttype) {
		case 1:
			colors[',']  = "\033[48;5;17m";  /* Blue background */
			colors['.']  = "\033[48;5;231m"; /* White stars */
			colors['\''] = "\033[48;5;16m";  /* Black border */
			colors['@']  = "\033[48;5;230m"; /* Tan poptart */
			colors['$']  = "\033[48;5;175m"; /* Pink poptart */
			colors['-']  = "\033[48;5;162m"; /* Red poptart */
			colors['>']  = "\033[48;5;196m"; /* Red rainbow */
			colors['&']  = "\033[48;5;214m"; /* Orange rainbow */
			colors['+']  = "\033[48;5;226m"; /* Yellow Rainbow */
			colors['#']  = "\033[48;5;118m"; /* Green rainbow */
			colors['=']  = "\033[48;5;33m";  /* Light blue rainbow */
			colors[';']  = "\033[48;5;19m";  /* Dark blue rainbow */
			colors['*']  = "\033[48;5;240m"; /* Gray cat face */
			colors['%']  = "\033[48;5;175m"; /* Pink cheeks */
			break;
		case 2:
			colors[',']  = "\033[104m";      /* Blue background */
			colors['.']  = "\033[107m";      /* White stars */
			colors['\''] = "\033[40m";       /* Black border */
			colors['@']  = "\033[47m";       /* Tan poptart */
			colors['$']  = "\033[105m";      /* Pink poptart */
			colors['-']  = "\033[101m";      /* Red poptart */
			colors['>']  = "\033[101m";      /* Red rainbow */
			colors['&']  = "\033[43m";       /* Orange rainbow */
			colors['+']  = "\033[103m";      /* Yellow Rainbow */
			colors['#']  = "\033[102m";      /* Green rainbow */
			colors['=']  = "\033[104m";      /* Light blue rainbow */
			colors[';']  = "\033[44m";       /* Dark blue rainbow */
			colors['*']  = "\033[100m";      /* Gray cat face */
			colors['%']  = "\033[105m";      /* Pink cheeks */
			break;
		case 3:
			colors[',']  = "\033[25;44m";    /* Blue background */
			colors['.']  = "\033[5;47m";     /* White stars */
			colors['\''] = "\033[25;40m";    /* Black border */
			colors['@']  = "\033[5;47m";     /* Tan poptart */
			colors['$']  = "\033[5;45m";     /* Pink poptart */
			colors['-']  = "\033[5;41m";     /* Red poptart */
			colors['>']  = "\033[5;41m";     /* Red rainbow */
			colors['&']  = "\033[25;43m";    /* Orange rainbow */
			colors['+']  = "\033[5;43m";     /* Yellow Rainbow */
			colors['#']  = "\033[5;42m";     /* Green rainbow */
			colors['=']  = "\033[25;44m";    /* Light blue rainbow */
			colors[';']  = "\033[5;44m";     /* Dark blue rainbow */
			colors['*']  = "\033[5;40m";     /* Gray cat face */
			colors['%']  = "\033[5;45m";     /* Pink cheeks */
			break;
		case 4:
			colors[',']  = "\033[0;34;44m";  /* Blue background */
			colors['.']  = "\033[1;37;47m";  /* White stars */
			colors['\''] = "\033[0;30;40m";  /* Black border */
			colors['@']  = "\033[1;37;47m";  /* Tan poptart */
			colors['$']  = "\033[1;35;45m";  /* Pink poptart */
			colors['-']  = "\033[1;31;41m";  /* Red poptart */
			colors['>']  = "\033[1;31;41m";  /* Red rainbow */
			colors['&']  = "\033[0;33;43m";  /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";  /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";  /* Green rainbow */
			colors['=']  = "\033[1;34;44m";  /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";  /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";  /* Gray cat face */
			colors['%']  = "\033[1;35;45m";  /* Pink cheeks */
			output = "██";
			break;
		case 5:
			colors[',']  = "\033[0;34;44m";  /* Blue background */
			colors['.']  = "\033[1;37;47m";  /* White stars */
			colors['\''] = "\033[0;30;40m";  /* Black border */
			colors['@']  = "\033[1;37;47m";  /* Tan poptart */
			colors['$']  = "\033[1;35;45m";  /* Pink poptart */
			colors['-']  = "\033[1;31;41m";  /* Red poptart */
			colors['>']  = "\033[1;31;41m";  /* Red rainbow */
			colors['&']  = "\033[0;33;43m";  /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";  /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";  /* Green rainbow */
			colors['=']  = "\033[1;34;44m";  /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";  /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";  /* Gray cat face */
			colors['%']  = "\033[1;35;45m";  /* Pink cheeks */
			output = "\333\333";
			break;
		case 6:
			colors[',']  = "::";             /* Blue background */
			colors['.']  = "@@";             /* White stars */
			colors['\''] = "  ";             /* Black border */
			colors['@']  = "##";             /* Tan poptart */
			colors['$']  = "??";             /* Pink poptart */
			colors['-']  = "<>";             /* Red poptart */
			colors['>']  = "##";             /* Red rainbow */
			colors['&']  = "==";             /* Orange rainbow */
			colors['+']  = "--";             /* Yellow Rainbow */
			colors['#']  = "++";             /* Green rainbow */
			colors['=']  = "~~";             /* Light blue rainbow */
			colors[';']  = "$$";             /* Dark blue rainbow */
			colors['*']  = ";;";             /* Gray cat face */
			colors['%']  = "()";             /* Pink cheeks */
			always_escape = 1;
			break;
		case 7:
			colors[',']  = ".";             /* Blue background */
			colors['.']  = "@";             /* White stars */
			colors['\''] = " ";             /* Black border */
			colors['@']  = "#";             /* Tan poptart */
			colors['$']  = "?";             /* Pink poptart */
			colors['-']  = "O";             /* Red poptart */
			colors['>']  = "#";             /* Red rainbow */
			colors['&']  = "=";             /* Orange rainbow */
			colors['+']  = "-";             /* Yellow Rainbow */
			colors['#']  = "+";             /* Green rainbow */
			colors['=']  = "~";             /* Light blue rainbow */
			colors[';']  = "$";             /* Dark blue rainbow */
			colors['*']  = ";";             /* Gray cat face */
			colors['%']  = "o";             /* Pink cheeks */
			always_escape = 1;
			terminal_width = 40;
			break;
		case 8:
			colors[',']  = "\033[48;2;0;49;105m";    /* Blue background */
			colors['.']  = "\033[48;2;255;255;255m"; /* White stars */
			colors['\''] = "\033[48;2;0;0;0m";       /* Black border */
			colors['@']  = "\033[48;2;255;205;152m"; /* Tan poptart */
			colors['$']  = "\033[48;2;255;169;255m"; /* Pink poptart */
			colors['-']  = "\033[48;2;255;76;152m";  /* Red poptart */
			colors['>']  = "\033[48;2;255;25;0m";    /* Red rainbow */
			colors['&']  = "\033[48;2;255;154;0m";   /* Orange rainbow */
			colors['+']  = "\033[48;2;255;240;0m";   /* Yellow Rainbow */
			colors['#']  = "\033[48;2;40;220;0m";    /* Green rainbow */
			colors['=']  = "\033[48;2;0;144;255m";   /* Light blue rainbow */
			colors[';']  = "\033[48;2;104;68;255m";  /* Dark blue rainbow */
			colors['*']  = "\033[48;2;153;153;153m"; /* Gray cat face */
			colors['%']  = "\033[48;2;255;163;152m"; /* Pink cheeks */
			break;
		default:
			break;
	}

	if (min_col == max_col) {
		min_col = (FRAME_WIDTH - terminal_width/2) / 2;
		max_col = (FRAME_WIDTH + terminal_width/2) / 2;
		using_automatic_width = 1;
	}

	if (min_row == max_row) {
		min_row = (FRAME_HEIGHT - (terminal_height-1)) / 2;
		max_row = (FRAME_HEIGHT + (terminal_height-1)) / 2;
		using_automatic_height = 1;
	}

	/* Attempt to set terminal title */
	if (set_title) {
		printf("\033kNyanyanyanyanyanyanya...\033\134");
		printf("\033]1;Nyanyanyanyanyanyanya...\007");
		printf("\033]2;Nyanyanyanyanyanyanya...\007");
	}

	if (clear_screen) {
		/* Clear the screen */
		printf("\033[H\033[2J\033[?25l");
	} else {
		printf("\033[s");
	}

	if (show_intro) {
		/* Display the MOTD */
		unsigned int countdown_clock = 5;
		for (k = 0; k < countdown_clock; ++k) {
			newline(3);
			printf("                             \033[1mNyancat Telnet Server\033[0m");
			newline(2);
			printf("                   written and run by \033[1;32mK. Lange\033[1;34m @_klange\033[0m");
			newline(2);
			printf("        If things don't look right, try:");
			newline(1);
			printf("                TERM=fallback telnet ...");
			newline(2);
			printf("        Or on Windows:");
			newline(1);
			printf("                telnet -t vtnt ...");
			newline(2);
			printf("        Problems? Check the website:");
			newline(1);
			printf("                \033[1;34mhttp://nyancat.dakko.us\033[0m");
			newline(2);
			printf("        This is a telnet server, remember your escape keys!");
			newline(1);
			printf("                \033[1;31m^]quit\033[0m to exit");
			newline(2);
			printf("        Starting in %d...                \n", countdown_clock-k);

			fflush(stdout);
			usleep(400000);
			if (clear_screen) {
				printf("\033[H"); /* Reset cursor */
			} else {
				printf("\033[u");
			}
		}

		if (clear_screen) {
			/* Clear the screen again */
			printf("\033[H\033[2J\033[?25l");
		}
	}

	/* Store the start time */
	time_t start, current;
	time(&start);

	int playing = 1;    /* Animation should continue [left here for modifications] */
	size_t i = 0;       /* Current frame # */
	unsigned int f = 0; /* Total frames passed */
	char last = 0;      /* Last color index rendered */
	int y, x;        /* x/y coordinates of what we're drawing */
	while (playing) {
		update_terminal_size(TERM_EVENT_WINDOW_RESIZED);
		if (using_automatic_width || using_automatic_height) {
			SIGWINCH_handler(0);
		}
		/* Reset cursor */
		if (clear_screen) {
			printf("\033[H");
		} else {
			printf("\033[u");
		}
		/* Render the frame */
		for (y = min_row; y < max_row; ++y) {
			for (x = min_col; x < max_col; ++x) {
				char color;
				if (y > 23 && y < 43 && x < 0) {
					/*
					 * Generate the rainbow tail.
					 *
					 * This is done with a pretty simplistic square wave.
					 */
					int mod_x = ((-x+2) % 16) / 8;
					if ((i / 2) % 2) {
						mod_x = 1 - mod_x;
					}
					/*
					 * Our rainbow, with some padding.
					 */
					const char *rainbow = ",,>>&&&+++###==;;;,,";
					color = rainbow[mod_x + y-23];
					if (color == 0) color = ',';
				} else if (x < 0 || y < 0 || y >= FRAME_HEIGHT || x >= FRAME_WIDTH) {
					/* Fill all other areas with background */
					color = ',';
				} else {
					/* Otherwise, get the color from the animation frame. */
					color = frames[i][y][x];
				}
				if (always_escape) {
					/* Text mode (or "Always Send Color Escapes") */
					printf("%s", colors[(int)color]);
				} else {
					if (color != last && colors[(int)color]) {
						/* Normal Mode, send escape (because the color changed) */
						last = color;
						printf("%s%s", colors[(int)color], output);
					} else {
						/* Same color, just send the output characters */
						printf("%s", output);
					}
				}
			}
			/* End of row, send newline */
			newline(1);
		}
		if (show_counter) {
			/* Get the current time for the "You have nyaned..." string */
			time(&current);
			double diff = difftime(current, start);
			/* Now count the length of the time difference so we can center */
			int nLen = digits((int)diff);
			/*
			 * 29 = the length of the rest of the string;
			 * XXX: Replace this was actually checking the written bytes from a
			 * call to sprintf or something
			 */
			int width = (terminal_width - 29 - nLen) / 2;
			/* Spit out some spaces so that we're actually centered */
			while (width > 0) {
				printf(" ");
				width--;
			}
			/* You have nyaned for [n] seconds!
			 * The \033[J ensures that the rest of the line has the dark blue
			 * background, and the \033[1;37m ensures that our text is bright white.
			 * The \033[0m prevents the Apple ][ from flipping everything, but
			 * makes the whole nyancat less bright on the vt220
			 */
			printf("\033[1;37mYou have nyaned for %0.0f seconds!\033[J\033[0m", diff);
		}
		/* Reset the last color so that the escape sequences rewrite */
		last = 0;
		/* Update frame count */
		++f;
		if (frame_count != 0 && f == frame_count) {
			finish();
		}
		++i;
		if (!frames[i]) {
			/* Loop animation */
			i = 0;
		}
		/* Wait */
		fflush(stdout);
		usleep(1000 * delay_ms);
	}
	return 0;
}
