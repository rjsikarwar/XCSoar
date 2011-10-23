#ifndef XCSOAR_SIZES_H
#define XCSOAR_SIZES_H

#define MINFREESTORAGE 500
// 500 kb must be free for logger to be active this is based on rough
// estimate that a long flight will detailed logging is about 200k,
// and we want to leave a little free.

// max length airspace and waypoint names
#define NAME_SIZE 50

// max length of waypoint comment names
#define COMMENT_SIZE 50

#define MAXSTARTPOINTS 10

#define MAX_LOADSTRING 100

#define NUMAIRSPACECOLORS 2
#define NUMAIRSPACEBRUSHES 8

// used by map window
#define WPCIRCLESIZE        2

#endif
