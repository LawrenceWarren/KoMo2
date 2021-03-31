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
#define CLIENT_STATE_BYPROG 0X44   // Program ended
#define CLIENT_STATE_RUNNING 0X80  // Program running
#define CLIENT_STATE_RUNNING_BL 0x81
#define CLIENT_STATE_RUNNING_SWI 0x81
#define CLIENT_STATE_STEPPING 0X82
#define CLIENT_STATE_BROKEN 0x30

#define RUN_FLAG_BL 0x02
#define RUN_FLAG_SWI 0x04
#define RUN_FLAG_ABORT 0x08
#define RUN_FLAG_BREAK 0x10
#define RUN_FLAG_WATCH 0x20
#define RUN_FLAG_INIT (RUN_FLAG_WATCH | RUN_FLAG_BREAK)

extern unsigned char board_runflags;

typedef struct {
  unsigned int misc;
  unsigned char addressA[8];  // ASSUMED fields large enough !!
  unsigned char addressB[8];
  unsigned char dataA[8];
  unsigned char dataB[8];
} trap_def;

typedef enum {  // Board instructions unsigned char
  BR_NOP = 0x00,
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

  BR_GET_REG = 0x5A,  // not general case!
  BR_GET_MEM = 0x48,
  BR_SET_REG = 0x52,  // not general case!
  BR_SET_MEM = 0x40,

  BR_WP_WRITE = 0x34,
  BR_WP_READ = 0x35,
  BR_WP_SET = 0x36,
  BR_WP_GET = 0x37,

  BR_START = 0x80
} BR_Instruction;

/**
 * @brief An imported source code (listing) line
 */
typedef struct source_line_name {
  struct source_line_name* pPrev;  // Previous line
  struct source_line_name* pNext;  // Next line
  int corrupt;                     // Flag if value changed
  int nodata;                      // Flag if line has no data fields
  unsigned int address;            // Address of entry
  int data_size[4];                // Sizes of fields
  int data_value[4];               // Data values
  char* text;                      // Text, as imported
} source_line;

typedef struct {
  source_line* pStart;  // First line in source (sorted into address order)
  source_line* pEnd;    // Last line in source
} source_file;

/**
 * @brief Stores the file descriptor used for writing to Jimulator.
 */
extern int writeToJimulator;
/**
 * @brief Stores the file descriptor used for reading from Jimulator.
 */
extern int readFromJimulator;
/**
 * @brief The pipe which will be used by KoMo2 to read from Jimulator (i.e.
 * Jimulator will write to it, KoMo2 will read)
 */
extern int communicationFromJimulator[2];
/**
 * @brief The pipe which will be used by KoMo2 to write to Jimulator (i.e.
 * Jimulator will read from it, KoMo2 will write)
 */
extern int communicationToJimulator[2];
/**
 * @brief The pipe that handles communication between the compiler process and
 * KoMo2.
 */
extern int compilerCommunication[2];