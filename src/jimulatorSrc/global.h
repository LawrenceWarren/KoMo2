/* Last modified: 20/7/2007                                                   */
/* Headers for shared definitions etc.                                        */
/* Use until we can get things properly scoped.     @@@                       */

// Get rid of this @@@
#ifndef GLOBAL_H
#define GLOBAL_H

#include <gdk/gdkx.h>
#include <gtk/gtk.h> /* GtkWidget used below */

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

/* Required by interface.h and view.h                                         */

typedef enum { /* this describes a type of a feature and will be used as member
                  in feature */
               UNKNOWN,
               XILINX_FPGA,
               TERMINAL,
               COUNTERS
} feature_type;

typedef struct { /* holds data of a Xilinx FPGA feature */
  char* filestring;
} xilinx_fpga_data;

typedef struct { /* holds data about a terminal */
  GtkWidget* text;
} terminal_data;

typedef struct { /* holds data about the counters */
  GtkWidget* display;
} counter_data;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Required by interface and callbacks                                        */

int emulator_PID;    /* @@@ not sure about scope */
char* compileScript; /* Filename of script attached to `compile' button */
                     // SHOULD be per architecture @@@@@

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Required by view, viewfuncs and callbacks                                  */

/* the following are the handles of the toggle-buttons used for flag display  */

GtkWidget* flag_button[2][7]; /* First index is CPSR/SPSR, second is flag # */
/* flag #: V, C, Z, N, I, F, T   ... turned out a bit silly @@@               */
GtkWidget* CPSR_menu_handle;
GtkWidget* SPSR_menu_handle;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

extern char* mem_column_data[]; /* Contents defined in callbacks.c */
                                /* Used in callbacks.c and view.c */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

GdkColor view_greycolour; /* defines the grey colour used in the application */
GdkColor view_redcolour;
GdkColor view_bluecolour;
GdkColor view_orangecolour;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#endif
