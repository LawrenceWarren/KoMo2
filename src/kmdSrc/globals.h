#include <gtk/gtk.h>

#define SOURCE_BYTE_COUNT 4
#define SOURCE_FIELD_COUNT 4
#define SOURCE_TEXT_LENGTH 100
#define SYM_BUF_SIZE 64
#define IN_POLL_TIMEOUT 1000
#define OUT_POLL_TIMEOUT 100
#define MAX_SERIAL_WORD 4

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

typedef enum { SCANSymbol, SCANString, SCANNumber, SCANList } SCANType;

typedef enum /* Internal symbol type; may want extending one day */
{ SYM_NONE,
  SYM_UNDEFINED,
  SYM_EQUATE,
  SYM_LOCAL,
  SYM_GLOBAL,
  SYM_SECTION,
  SYM_WEAK } symbol_type;

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

typedef struct SCANNode {
  SCANType type;
  int line;
  int position;
  int list_type;
  union {
    char* string; /* String and (or??) Symbol */
    int number;
    GList* list;
  } body;
} SCANNode, *PtrSCANNode;

typedef struct feature_name { /* Feature struct */
  unsigned char reference_number;
  int sub_reference_number;
  char* name; /* its name as recognised from .komodo file */
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

typedef struct memory_segment_name { /* Memory Segments */
  unsigned char*
      start; /*chararr ptr to addr of length board->memory_ptr_width */
  unsigned char* length; /* chararr of length board->memory_ptr_width */
} memory_segment;

typedef struct reg_bank_name { /* Regbank struct */
  char* name;                  /* Name to display in window */
  int offset;                  /* Offset in registers (in bits if flags) */
  unsigned char width;         /* Width in bytes (0 means 1bit flags) */
  unsigned char number;        /* Count of registers */
  GList** names;               /* Array of lists of register names */
  unsigned char*
      values;  /* array of values (regvalue = values+regnumber*width */
  int pointer; /* does this regbank store memory pointers */
  int valid;   /* Indicates that registers have been fetched */
} reg_bank;    /* flags use 1 char each */

typedef struct DefinitionStackName {
  struct DefinitionStackName* next;
  char* string;
  GList* rules;
} DefinitionStack;

typedef struct target_system_name { /* Record defining back-end
                                     * characteristics
                                     */
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
  int regbanks_gran;            /* Granularity of the register banks */
  reg_bank* reg_banks;          /* Pointer to an array of `reg_bank' records */
                                // Space allocated
  DefinitionStack* asm_tables;  // Think this uses someone else's space @@
} target_system;

typedef struct symbol_name {
  char* name; /* Pointer to symbol name string */
  long value; /* Symbol's value */
  symbol_type
      sym_type; /* Some indication of the symbol's means of definition */
  struct symbol_name* pDef; /* Next defined symbol */
  //  struct symbol_name *pAlpha;                   /* Alphabetically next
  //  symbol */ struct symbol_name *pVal;                             /* Next
  //  valued symbol */
} symbol;

typedef struct source_line_name /* An imported source code (listing) line */
{
  struct source_line_name* pPrev; /* Previous line */
  struct source_line_name* pNext; /* Next line */
  int corrupt;                    /* Flag if value changed */
  int nodata;                     /* Flag if line has no data fields */
  unsigned int address;           /* Address of entry */
  int data_size[4];               /* Sizes of fields */
  int data_value[4];              /* Data values */
  char* text;                     /* Text, as imported */
} source_line;

typedef struct {
  source_line* pStart; /* First line in source (sorted into address order) */
  source_line* pEnd;   /* Last line in source */
} source_file;

// extern source_file kmdSourceFile;
// extern target_system* board; // TODO: make board information constant

extern symbol* symbol_table;  // A symbol table that is made up of symbols
extern int symbol_count;      // TODO: increment this (miscAddSymbol)
extern int writeToJimulator;
extern int readFromJimulator;
extern int emulator_PID;
extern int board_emulation_communication_from[2];
extern int board_emulation_communication_to[2];
