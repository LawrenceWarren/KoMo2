/* ':':':':':':':':':'@' '@':':':':':':':':':'@' '@':':':':':':':':':'| */
/* :':':':':':':':':':'@'@':':':':':':':':':':'@'@':':':':':':':':':':| */
/* ':':':': : :':':':':'@':':':':': : :':':':':'@':':':':': : :':':':'| */
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/*                Name:           interface.h                           */
/*                Version:        1.5.0                                 */
/*                Date:           20/07/2007                            */
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

#ifndef INTERFACE_H
#define INTERFACE_H
#define KNOWN_CPUS 1
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "chump.h"
#include "global.h"

#define _(String) (String)

#define MAX_SERIAL_WORD 4
/* Currently the maximum size of a word sent to emulator or board */

#define SPECIAL_REGISTER_COUNT 3 /* "Special registers" are SP, LR, PC */

/* The following define possible states of the board */

#define CLIENT_STATE_CLASS_MASK 0XC0
#define CLIENT_STATE_CLASS_RESET 0X00
#define CLIENT_STATE_CLASS_STOPPED 0X40
#define CLIENT_STATE_CLASS_RUNNING 0X80
#define CLIENT_STATE_CLASS_ERROR 0XC0
#define CLIENT_STATE_RESET 0X00
#define CLIENT_STATE_BUSY 0X01
#define CLIENT_STATE_STOPPED 0X40
#define CLIENT_STATE_BREAKPOINT 0X41
#define CLIENT_STATE_WATCHPOINT 0X42
#define CLIENT_STATE_MEMFAULT 0X43
#define CLIENT_STATE_BYPROG 0X44
#define CLIENT_STATE_RUNNING 0X80
#define CLIENT_STATE_RUNNING_BL 0x81  //  @@@ NEEDS UPDATE
#define CLIENT_STATE_RUNNING_SWI 0x81
#define CLIENT_STATE_STEPPING 0X82

#define CLIENT_STATE_BROKEN 0x30

#define RUN_FLAG_BL 0x02
#define RUN_FLAG_SWI 0x04
#define RUN_FLAG_ABORT 0x08
#define RUN_FLAG_BREAK 0x10
#define RUN_FLAG_WATCH 0x20
#define RUN_FLAG_INIT (RUN_FLAG_WATCH | RUN_FLAG_BREAK)

typedef enum { GDK_FONT_FONT, GDK_FONT_FONTSET } GdkFontType;

typedef struct _GdkFont {
  GdkFontType type;
  gint ascent;
  gint descent;
} GdkFont;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* The following defines possible instructions that can be sent to the board  */
/*   as 1 byte each.                                                          */

typedef enum {                /* Board instructions unsigned char */
               BR_NOP = 0x00, /* need work on */
               BR_PING = 0x01,
               BR_WOT_R_U = 0x02,
               BR_RESET = 0x04,
               BR_COMM_W = 0x06,
               BR_COMM_R = 0x07,

               BR_FR_GET = 0x10,
               BR_FR_SET = 0x11,
               BR_FR_WRITE = 0x12,
               BR_FR_READ = 0x13,
               BR_FR_FILE = 0x14,
               BR_FR_SEND = 0x15,

               BR_WOT_U_DO = 0x20,
               BR_STOP = 0x21,
               BR_PAUSE = 0x22,
               BR_CONTINUE = 0x23,

               BR_RTF_SET = 0x24,
               BR_RTF_GET = 0x25,

               BR_BP_WRITE = 0x30,
               BR_BP_READ = 0x31,
               BR_BP_SET = 0x32,
               BR_BP_GET = 0x33,

               BR_GET_REG = 0x5A, /* not general case! */
               BR_GET_MEM = 0x48,
               BR_SET_REG = 0x52, /* not general case! */
               BR_SET_MEM = 0x40,

               BR_WP_WRITE = 0x34,
               BR_WP_READ = 0x35,
               BR_WP_SET = 0x36,
               BR_WP_GET = 0x37,

               BR_START = 0x80
} BR_Instruction;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/**
 * @brief Feature struct
 *
 */
typedef struct feature {
  unsigned char reference_number;
  int sub_reference_number;
  char* name;  // its name as recognised from .komodo file
  feature_type type;
  /* see above - enumeration with different supported features */
  union { /* the data within that feature */
    xilinx_fpga_data xilinx_fpga;
    terminal_data terminal;
    counter_data counter;
  } data;
  unsigned char dev_number;
  /* number to communicate by - serial number assigned depending on the order */
} feature; /*   the board sent it */

/**
 * @brief Memory Segments
 *
 */
typedef struct memory_segment {
  unsigned char*
      start; /*chararr ptr to addr of length board->memory_ptr_width */
  unsigned char* length; /* chararr of length board->memory_ptr_width */
} memory_segment;

/**
 * @brief Regbank struct
 *
 */
typedef struct reg_bank {
  char* name;                  // Name to display in window
  int offset;                  // Offset in registers (in bits if flags)
  unsigned char widthInBytes;  // Width in bytes (0 means 1bit flags)
  unsigned char numberOfRegisters;
  GList** individualRegisterNames;
  unsigned char* values;  // array of values (regvalue = values+regnumber*width
  int pointer;            // does this regbank store memory pointers
  bool valid;             // Indicates that registers have been fetched
} reg_bank;

/**
 * @brief
 * special_reg struct
 */
typedef struct special_reg {
  char* name;           /* Name of register */
  unsigned char* value; /* offset of value */
  GdkColor colour;
  char** pixmap_data;
  cairo_surface_t* pixmap;
  cairo_surface_t* bitmap;
  bool active; /* if activated or not */
  int* valid;
} special_reg; /* flags use 1 char each */

/**
 * @brief A record containing back-end characteristics
 *
 */
typedef struct target_system {
  unsigned char cpu_ref;
  int cpu_subref;
  char* cpu_name;  // Points to someone else's space
  unsigned int feature_count;
  feature* pArray_F;  // Space allocated
  unsigned int memory_count;
  memory_segment* pArray_M;  // Space allocated
  int memory_ptr_width;
  int wordalign;
  int num_regbanks;
  int registerBankGranularity;
  reg_bank* arrayOfRegisterBanks;  // Pointer to an array of `reg_bank' records
  DefinitionStack* asm_tables;     // Think this uses someone else's space
} target_system;

typedef enum { SERIAL, NETWORK, EMULATOR, FAKE } target_type;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

char* rcfile;      /* The name of the file to be opened - .komodo */
bool use_internal; /* ... config. file */
bool VERBOSE;      /* Be loud and informative or not */

GtkStyle* fixed_style;

target_type
    interface_type; /* Indicates what board/virtual board we work with */
int board_version;  /* retains the version of the board */
                    /* -2 = uninitialised, -1 = broken */

target_system* board; /* Pointer to record holding current backend details */

special_reg
    special_registers[SPECIAL_REGISTER_COUNT];  // #!#!# Should go in `board'
                                                // record too ???@@@

/***************************/
/** Locking               **/
/***************************/

/* The following is a lock definition that is useable for different locks     */
typedef enum {
  FREE,
  TERMINAL_UPDATE,
  MEM_AND_REG_UPDATE,
  FPGA_LOAD,
  LOAD,
  COMPILE
} lock_state;

lock_state serial_lock;  // this is a variable to hold the lock state
lock_state fpga_lock;
lock_state compilerLock;
lock_state load_lock;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool board_mini_ping(void);
bool board_micro_ping(void);  /* micro is even faster */
int board_enq(unsigned int*); /* Ask the board what it is doing */
int board_get_regbank(int); /* Update the info on all registers in that bank */
int board_set_register(int, int, unsigned char*);
/* Set one register (regbank,regnumber,value) */
bool board_get_memory(int, unsigned char*, unsigned char*, int);
/* Get memory (count,address,destination, size) */
bool board_set_memory(int, unsigned char*, unsigned char*, int);
/* Set memory (count,address,source, size) */

/***************************/
/** breakpoints functions **/
/***************************/

typedef struct {
  unsigned int misc;
  unsigned char addressA[8];
  unsigned char addressB[8];
  unsigned char dataA[8];
  unsigned char dataB[8];
} trap_def;

bool trap_get_status(unsigned int trap_type,
                     unsigned int* wordA,
                     unsigned int* wordB);
void trap_set_status(unsigned int trap_type,
                     unsigned int wordA,
                     unsigned int wordB);
bool read_trap_defn(unsigned int trap_type,
                    unsigned int trap_number,
                    trap_def* trap_defn);
bool write_trap_defn(unsigned int trap_type,
                     unsigned int trap_number,
                     trap_def* trap_defn);

/*****************************/
/** Board control functions **/
/*****************************/

void stop_board(void);
void continue_board(void);
void reset_board(void);
void set_interrupt_service(int, int);
/* int  run_board(int steps); */
bool run_board(int* steps); /* Interface changed (corrected) 21/6/06 */
void walk_board(int, int);
void change_start_command(int, int);
bool test_run_flag(unsigned int);
void verbosePrint(const char* userData, ...);

/*****************************/
/** features functions      **/
/*****************************/

void read_string_terminal(gpointer);
// void send_key_to_terminal(int, gpointer);
void send_key_to_terminal(int, int);
char download_send_header(int, int);
char download_send_packet(int, unsigned int, unsigned char*);

#endif

/*                                                                      */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*                     end of interface.h                               */
/************************************************************************/
