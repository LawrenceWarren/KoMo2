/* Last modified: 20/7/2007                                                   */
/* Headers for shared definitions etc.                                        */
/* Use until we can get things properly scoped.     @@@                       */

// Get rid of this @@@
#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_WORD_LENGTH 4 /* Space reserved in `character arrays' for words */

#define MEM_WINDOW_X 1000  // These need reconciling @@@
#define MEM_WINDOW_Y 600

#define MEM_WINDOW_BASE_WIDTH 200
#define MEM_WINDOW_BASE_HEIGHT 100
#define MEM_WINDOW_MIN_WIDTH 200
#define MEM_WINDOW_MIN_HEIGHT 100
#define MEM_WINDOW_MAX_WIDTH 1280
#define MEM_WINDOW_MAX_HEIGHT 1000

#define SOURCE_WINDOW_BASE_WIDTH 400
#define SOURCE_WINDOW_BASE_HEIGHT 100
#define SOURCE_WINDOW_MIN_WIDTH 400
#define SOURCE_WINDOW_MIN_HEIGHT 100
#define SOURCE_WINDOW_MAX_WIDTH 700
#define SOURCE_WINDOW_MAX_HEIGHT 1000

#define SYMBOL_WINDOW_BASE_WIDTH 300
#define SYMBOL_WINDOW_BASE_HEIGHT 50
#define SYMBOL_WINDOW_MIN_WIDTH 100
#define SYMBOL_WINDOW_MIN_HEIGHT 50
#define SYMBOL_WINDOW_MAX_WIDTH 700
#define SYMBOL_WINDOW_MAX_HEIGHT 1000

// #define THUMB				// Bodge to get things going @@@

#define SOURCE_EXT ".s"
#define OBJECT_EXT ".kmd"
//#define OBJECT_EXT  ".elf"
#define BITFILE_EXT ".bit"

int TRACE; /* Value - print trace of major procedure calls higher => more */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Required by interface and callbacks                                        */

int emulator_PID;    /* @@@ not sure about scope */
char* compileScript; /* Filename of script attached to `compile' button */
                     // SHOULD be per architecture @@@@@

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

extern char* mem_column_data[]; /* Contents defined in callbacks.c */
                                /* Used in callbacks.c and view.c */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#endif
