#ifndef DEF_H__
#define DEF_H__

#include <ncurses.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, n, max) MIN(MAX((min), (n)), (max))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) / ABS(n))

// global editor configuration.
#define GLOBAL_NORM_PAIR 1
#define GLOBAL_HIGHLIGHT_PAIR 2
#define GLOBAL_NORM_BG COLOR_BLACK
#define GLOBAL_NORM_FG COLOR_YELLOW
#define GLOBAL_HIGHLIGHT_BG COLOR_YELLOW
#define GLOBAL_HIGHLIGHT_FG COLOR_BLACK

#endif
