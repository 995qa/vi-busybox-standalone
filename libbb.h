#ifndef LIBBB_H
#define LIBBB_H

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
#include <poll.h>
#include <fcntl.h>
#include <stddef.h>  // Added: Provides offsetof
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

// BusyBox-specific types & structural macros
typedef int8_t smallint;
typedef uint8_t smalluint;

#define ALIGN1 __attribute__((aligned(1)))
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define UNUSED_PARAM __attribute__((__unused__))  // Added: Fixes signal handler syntax errors

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))  // Added: Array size utility

#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE 1
#endif

#define CONFIG_FEATURE_VI_MAX_LEN 4096
#define KEYCODE_BUFFER_SIZE 16
#define CONFIG_FEATURE_VI_UNDO_QUEUE_MAX 256
#define BB_VER "1.36.0-standalone"  // Added: Version string

// Feature Configuration (All enabled for maximum capability)
#define ENABLE_FEATURE_VI_8BIT 0
#define ENABLE_FEATURE_VI_COLON 1
#define ENABLE_FEATURE_VI_YANKMARK 1
#define ENABLE_FEATURE_VI_SEARCH 1
#define ENABLE_FEATURE_VI_REGEX_SEARCH 0
#define ENABLE_FEATURE_VI_USE_SIGNALS 0
#define ENABLE_FEATURE_VI_DOT_CMD 0
#define ENABLE_FEATURE_VI_READONLY 1
#define ENABLE_FEATURE_VI_SETOPTS 0
#define ENABLE_FEATURE_VI_SET 0
#define ENABLE_FEATURE_VI_WIN_RESIZE 0
#define ENABLE_FEATURE_VI_ASK_TERMINAL 0
#define ENABLE_FEATURE_VI_UNDO 1
#define ENABLE_FEATURE_VI_UNDO_QUEUE 1
#define ENABLE_FEATURE_VI_CRASHME 0
#define ENABLE_LOCALE_SUPPORT 1

// Macro helper expansions (Solves the "expected ')'" syntax errors)
#define IF_FEATURE_VI_READONLY(...) __VA_ARGS__
#define IF_FEATURE_VI_COLON(...) __VA_ARGS__
#define IF_FEATURE_VI_YANKMARK(...) __VA_ARGS__
#define IF_FEATURE_VI_SEARCH(...) __VA_ARGS__
#define IF_FEATURE_VI_REGEX_SEARCH(...) __VA_ARGS__
#define IF_FEATURE_VI_USE_SIGNALS(...) __VA_ARGS__
#define IF_FEATURE_VI_DOT_CMD(...) __VA_ARGS__
#define IF_FEATURE_VI_SETOPTS(...) __VA_ARGS__
#define IF_FEATURE_VI_SET(...) __VA_ARGS__
#define IF_FEATURE_VI_WIN_RESIZE(...) __VA_ARGS__
#define IF_FEATURE_VI_ASK_TERMINAL(...) __VA_ARGS__
#define IF_FEATURE_VI_UNDO(...) __VA_ARGS__
#define IF_FEATURE_VI_UNDO_QUEUE(...) __VA_ARGS__
#define IF_FEATURE_VI_CRASHME(...)

// Custom BusyBox Keycodes
enum {
    KEYCODE_UP         = -2,
    KEYCODE_DOWN       = -3,
    KEYCODE_RIGHT      = -4,
    KEYCODE_LEFT       = -5,
    KEYCODE_HOME       = -6,
    KEYCODE_END        = -7,
    KEYCODE_INSERT     = -8,
    KEYCODE_DELETE     = -9,
    KEYCODE_PAGEUP     = -10,
    KEYCODE_PAGEDOWN   = -11,
    KEYCODE_CURSOR_POS = -12, // Added: Terminal cursor position response
};

// Global Pointer logic mappings
struct globals;
extern struct globals *ptr_to_globals;
#define SET_PTR_TO_GLOBALS(x) (ptr_to_globals = (x))
#define MAIN_EXTERNALLY_VISIBLE

extern uint32_t option_mask32;
extern const char *applet_name;

// Character processing wrappers
static inline int isprint_asciionly(int c) {
    return (c >= 0x20 && c < 0x7f);
}

static inline char* skip_whitespace(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return (char*)s;
}

static inline char* skip_non_whitespace(const char *s) {
    while (*s && !isspace((unsigned char)*s)) s++;
    return (char*)s;
}

// Memory allocators
static inline void* xzalloc(size_t size) {
    void *p = calloc(1, size);
    if (!p && size) {
        perror("calloc");
        exit(1);
    }
    return p;
}

static inline void* xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p && size) {
        perror("malloc");
        exit(1);
    }
    return p;
}

static inline void* xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p && size) {
        perror("realloc");
        exit(1);
    }
    return p;
}

static inline char* xstrdup(const char *s) {
    if (!s) return NULL;
    char *p = strdup(s);
    if (!p) {
        perror("strdup");
        exit(1);
    }
    return p;
}

static inline char* xstrndup(const char *s, size_t n) {
    char *p = strndup(s, n);
    if (!p && n) {
        perror("strndup");
        exit(1);
    }
    return p;
}

static inline char* xasprintf(const char *format, ...) {
    char *p = NULL;
    va_list ap;
    va_start(ap, format);
    int r = vasprintf(&p, format, ap);
    va_end(ap);
    if (r < 0 || !p) {
        perror("vasprintf");
        exit(1);
    }
    return p;
}

// Basic output shims
#define fputs_stdout(s) fputs(s, stdout)
#define fflush_all() fflush(NULL)
#define bb_putchar(c) putchar(c)

static inline void bb_simple_error_msg_and_die(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    exit(1);
}

static inline void bb_show_usage(void) {
    fprintf(stderr, "Usage: vi [FILE]...\n");
    exit(1);
}

#define STRERROR_FMT ": %s"
#define STRERROR_ERRNO , strerror(errno)

// Core reading/writing implementations (safe against interrupts)
static inline ssize_t safe_read(int fd, void *buf, size_t count) {
    ssize_t r;
    do {
        r = read(fd, buf, count);
    } while (r < 0 && errno == EINTR);
    return r;
}

static inline ssize_t full_read(int fd, void *buf, size_t len) {
    size_t cc = 0;
    while (len > 0) {
        ssize_t r = safe_read(fd, buf, len);
        if (r < 0) return r;
        if (r == 0) break;
        buf = (char *)buf + r;
        len -= r;
        cc += r;
    }
    return cc;
}

static inline ssize_t safe_write(int fd, const void *buf, size_t count) {
    ssize_t r;
    do {
        r = write(fd, buf, count);
    } while (r < 0 && errno == EINTR);
    return r;
}

static inline ssize_t full_write(int fd, const void *buf, size_t len) {
    size_t cc = 0;
    while (len > 0) {
        ssize_t r = safe_write(fd, buf, len);
        if (r < 0) return r;
        if (r == 0) break;
        buf = (const char *)buf + r;
        len -= r;
        cc += r;
    }
    return cc;
}

// Query terminal dimension helper
/*static inline int get_terminal_width_height(int fd, int *width, int *height) {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        if (width) *width = ws.ws_col;
        if (height) *height = ws.ws_row;
        return 0;
    }
    if (width) *width = 80;
    if (height) *height = 24;
    return -1;
}*/
static inline int get_terminal_width_height(int fd, unsigned *width, unsigned *height) {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        if (width) *width = ws.ws_col;
        if (height) *height = ws.ws_row;
        return 0;
    }
    if (width) *width = 80;
    if (height) *height = 24;
    return -1;
}

// Custom Key-read parser for handling ANSI escape sequences (Arrow keys, etc.)
static inline int32_t safe_read_key(int fd, char *buffer, int timeout) {
    unsigned char c;
    if (safe_read(fd, &c, 1) <= 0) {
        return -1;
    }

    if (c == 27) { // Escape Sequence
        struct pollfd pfd = { fd, POLLIN, 0 };
        if (poll(&pfd, 1, 50) > 0) {
            unsigned char seq[4];
            int n = read(fd, seq, sizeof(seq));
            if (n > 0) {
                if (seq[0] == '[') {
                    if (n >= 2) {
                        switch (seq[1]) {
                            case 'A': return KEYCODE_UP;
                            case 'B': return KEYCODE_DOWN;
                            case 'C': return KEYCODE_RIGHT;
                            case 'D': return KEYCODE_LEFT;
                            case 'H': return KEYCODE_HOME;
                            case 'F': return KEYCODE_END;
                        }
                    }
                    if (n >= 3 && seq[2] == '~') {
                        switch (seq[1]) {
                            case '1': return KEYCODE_HOME;
                            case '2': return KEYCODE_INSERT;
                            case '3': return KEYCODE_DELETE;
                            case '4': return KEYCODE_END;
                            case '5': return KEYCODE_PAGEUP;
                            case '6': return KEYCODE_PAGEDOWN;
                        }
                    }
                } else if (seq[0] == 'O') {
                    if (n >= 2) {
                        switch (seq[1]) {
                            case 'H': return KEYCODE_HOME;
                            case 'F': return KEYCODE_END;
                        }
                    }
                }
            }
        }
    }
    return c;
}

// Polling and Terminal setup
static inline int safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout) {
    int r;
    do {
        r = poll(ufds, nfds, timeout);
    } while (r < 0 && (errno == EINTR || errno == EAGAIN));
    return r;
}

#define TERMIOS_RAW_CRNL 1
static inline void set_termios_to_raw(int fd, struct termios *old, int flags) {
    struct termios raw;
    tcgetattr(fd, old);
    raw = *old;
    cfmakeraw(&raw);
    if (flags & TERMIOS_RAW_CRNL) {
        raw.c_iflag |= ICRNL;
        raw.c_oflag |= ONLCR;
    }
    tcsetattr(fd, TCSANOW, &raw);
}

static inline void tcsetattr_stdin_TCSANOW(struct termios *t) {
    tcsetattr(STDIN_FILENO, TCSANOW, t);
}

// BusyBox CLI argument parsing mock
static inline uint32_t getopt32(char **argv, const char *applet_opts, ...) {
    int i = 1;
    while (argv[i]) {
        if (argv[i][0] == '-') {
            // Basic parameter bypass logic
        } else {
            break;
        }
        i++;
    }
    optind = i;
    return 0;
}

// BusyBox string options array searcher
static inline int index_in_strings(const char *strings, const char *key) {
    int idx = 0;
    while (*strings) {
        if (strcmp(strings, key) == 0) {
            return idx;
        }
        strings += strlen(strings) + 1;
        idx++;
    }
    return -1;
}

static inline unsigned bb_strtou(const char *arg, char **endp, int base) {
    return (unsigned)strtoul(arg, endp, base);
}

// Custom simple linked list mapping
typedef struct llist_t {
    struct llist_t *link;
    char *data;
} llist_t;

static inline void* llist_pop(llist_t **head) {
    if (!head || !*head) return NULL;
    llist_t *curr = *head;
    void *data = curr->data;
    *head = curr->link;
    free(curr);
    return data;
}

static inline void llist_add_to(llist_t **head, void *data) {
    llist_t *new_node = (llist_t*)xmalloc(sizeof(llist_t));
    new_node->data = (char*)data;
    new_node->link = *head;
    *head = new_node;
}

static inline void llist_add_to_end(llist_t **head, void *data) {
    llist_t *new_node = (llist_t*)xmalloc(sizeof(llist_t));
    new_node->data = (char*)data;
    new_node->link = NULL;
    if (!*head) {
        *head = new_node;
        return;
    }
    llist_t *curr = *head;
    while (curr->link) {
        curr = curr->link;
    }
    curr->link = new_node;
}

// File and path system helpers
static inline char* concat_path_file(const char *path, const char *filename) {
    char *p;
    if (!path || !*path) return xstrdup(filename);
    if (path[strlen(path)-1] == '/') {
        p = xasprintf("%s%s", path, filename);
    } else {
        p = xasprintf("%s/%s", path, filename);
    }
    return p;
}

static inline char* xmalloc_open_read_close(const char *filename, size_t *maxsz) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return NULL;
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }
    size_t size = st.st_size;
    if (maxsz && size > *maxsz) size = *maxsz;
    char *buf = (char*)xmalloc(size + 1);
    ssize_t r = full_read(fd, buf, size);
    close(fd);
    if (r < 0) {
        free(buf);
        return NULL;
    }
    buf[r] = '\0';
    if (maxsz) *maxsz = r;
    return buf;
}

#endif // LIBBB_H
