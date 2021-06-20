/******************************************************************************/
/*                Name:           jimulator.c                                 */
/*                Version:        1.5.1                                       */
/*                Date:           5/08/2008                                   */
/*                Emulation library for KMD                                   */
/*                                                                            */
/*============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <iostream>

#include "definitions.h"
#include "interface.h"

#define NO_OF_BREAKPOINTS 32  // Max 32
#define NO_OF_WATCHPOINTS 4   // Max 32

/*  uses JDG's arm_v3

 notes:

 random changed to rand to allow linking
 N.B. check rand as may not produce 32-bit no.s


 needs:

 1. long multiply  - drafted BUT NOT TESTED
 3. more thumb testing

 lsl routine added but not fully tested (28/10/98)
 Long multiplication written (27/1/00)
 Architecture V5 added (2/2/00)

 To do:
 check long muls
 Flag checking (immediates ?!)
 Validation
 interrupt enable behaviour on exceptions (etc.)
 */

/* NB "int" is assumed to be at least 32-bit */

#define RING_BUF_SIZE 64

typedef struct {
  uint iHead;
  uint iTail;
  unsigned char buffer[RING_BUF_SIZE];
} ringBuffer;

struct pollfd pollfd;

// Local prototypes

void step();
void comm(struct pollfd*);

void emulsetup();
void saveState(unsigned char new_status);
void initialise(uint start_address, int initial_mode);
void execute(uint op_code);

// ARM execute

void dataOp(uint op_code);
void clz(uint op_code);
void transfer(uint op_code);
void transfer_sbhw(uint op_code);
void multiple(uint op_code);
void branch(uint op_code);
void mySystem(uint op_code);
void undefined();
void breakpoint();

void mrs(uint op_code);
void msr(uint op_code);
void bx(uint Rm, int link);
void myMulti(uint op_code);
void swap(uint op_code);
void normalDataOp(uint op_code, int operation);
void ldm(int mode, int Rn, int reg_list, bool write_back, bool hat);
void stm(int mode, int Rn, int reg_list, bool write_back, bool hat);

int checkWatchpoints(uint address, int data, int size, int direction);
int transferOffset(int op2, int add, int imm, bool sbhw);

int bReg(int op2, int* cf);
int bImmediate(int op2, int* cf);

int bitCount(uint source, int* first);
bool checkCC(int condition);

constexpr const bool zf(const int cpsr);
constexpr const bool cf(const int cpsr);
constexpr const bool nf(const int cpsr);
constexpr const bool vf(const int cpsr);

void setFlags(int operation, int a, int b, int rd, int carry);
void setNZ(uint value);
void setCF(uint a, uint rd, int carry);
void setVF_ADD(int a, int b, int rd);
void setVF_SUB(int a, int b, int rd);
int getRegister(int reg_no, int force_mode);
/* Returns PC+4 for ARM & PC+2 for Thumb */
int getRegisterMonitor(int reg_no, int force_mode);
void putRegister(int reg_no, int value, int force_mode);
constexpr const int instructionLength(const int cpsr, const int tf_mask);

uint fetch();
void incPC();
void endianSwap(uint start, uint end);
int readMemory(uint address, int size, bool sign, bool T, int source);
void writeMemory(uint address, int data, int size, bool T, int source);

/* THUMB execute */
void data0(uint op_code);
void data1(uint op_code);
void data_transfer(uint op_code);
void transfer0(uint op_code);
void transfer1(uint op_code);
void sp_pc(uint op_code);
void lsm_b(uint op_code);
void thumb_branch(uint op_code);

int loadFPE();
void FPEInstall();

int getNumber(char* ptr);
int lsl(int value, int distance, int* cf);
int lsr(uint value, int distance, int* cf);
int asr(int value, int distance, int* cf);
int ror(uint value, int distance, int* cf);

uint getmem32(int number);
void setmem32(int number, uint reg);
void executeInstruction();

int getChar(unsigned char* to_get);
int sendChar(unsigned char to_send);
int sendNBytes(int value, int N);
int getNBytes(int* val_ptr, int N);
int getCharArray(int char_number, unsigned char* data_ptr);
int sendCharArray(int char_number, unsigned char* data_ptr);

void boardreset();

void initBuffer(ringBuffer*);
int countBuffer(ringBuffer*);
int putBuffer(ringBuffer*, unsigned char);
int getBuffer(ringBuffer*, unsigned char*);

// Why add "RAMSIZE", and then get it wrong?!?!
// Memory is modulo this to the monitor; excise and use the proper routines
#define mem_size 0X100000      // 4Mbytes
#define RAMSIZE 0X100000       // 4Mbytes
#define reserved_mem 0X002000  // 32Kbytes
#define user_stack (mem_size - reserved_mem) << 2
#define start_string_addr 0X00007000  // ARM address

#define max_instructions 10000000

const uint nf_mask = 0X80000000;
const uint zf_mask = 0X40000000;
const uint cf_mask = 0X20000000;
const uint vf_mask = 0X10000000;
const uint if_mask = 0X00000080;
const uint ff_mask = 0X00000040;
const uint mode_mask = 0X0000001F;
const uint tf_mask = 0X00000020;  // THUMB bit

const uint bit_31 = 0X80000000;
const uint bit_0 = 0X00000001;

const uint imm_mask = 0X02000000;       // orginal word versions
const uint imm_hw_mask = 0X00400000;    // half word versions
const uint data_op_mask = 0X01E00000;   // ALU function code
const uint data_ext_mask = 0X01900000;  // To sort out CMP from MRS
const uint arith_ext = 0X01000000;      // Poss. arithmetic extension
const uint s_mask = 0X00100000;
const uint rn_mask = 0X000F0000;
const uint rd_mask = 0X0000F000;
const uint rs_mask = 0X00000F00;
const uint rm_mask = 0X0000000F;
const uint op2_mask = 0X00000FFF;
const uint hw_mask = 0X00000020;
const uint sign_mask = 0X00000040;

const uint mul_mask = 0X0FC000F0;
const uint long_mul_mask = 0X0F8000F0;
const uint mul_op = 0X00000090;
const uint long_mul_op = 0X00800090;
const uint mul_acc_bit = 0X00200000;
const uint mul_sign_bit = 0X00400000;
const uint mul_long_bit = 0X00800000;

const uint sbhw_mask = 0X0E000FF0;

const uint swp_mask = 0X0FB00FF0;
const uint swp_op = 0X01000090;

const uint pre_mask = 0X01000000;
const uint up_mask = 0X00800000;
const uint byte_mask = 0X00400000;
const uint write_back_mask = 0X00200000;
const uint load_mask = 0X00100000;
const uint byte_sign = 0X00000080;
const uint hw_sign = 0X00008000;

const uint user_mask = 0X00400000;

constexpr const uint link_mask = 0X01000000;
constexpr const uint branch_field = 0X00FFFFFF;
constexpr const uint branch_sign = 0X00800000;

constexpr const uint undef_mask = 0X0E000010;
constexpr const uint undef_code = 0X06000010;

constexpr const int memSystem = 0;  // sources for memory read
constexpr const int memInstruction = 1;
constexpr const int memData = 2;

constexpr const int flagAdd = 1;
constexpr const int flagSub = 2;

#define userMode 0X00000010
#define fiqMode 0X00000011
#define irqMode 0X00000012
#define supMode 0X00000013
#define abtMode 0X00000017
#define undefMode 0X0000001B
#define systemMode 0X0000001F

#define reg_current 0  //  Values to force register accesses to specified bank
#define reg_user 1     // or system
#define regSvc 2
#define regFiq 3
#define regIrq 4
#define regAbt 5
#define reg_undef 6

#define REGSIZE 65536
#define uchar unsigned char

typedef struct {
  int state;
  unsigned char cond;
  unsigned char size;
  int addra;
  int addrb;
  int dataa[2];
  int datab[2];
} BreakElement;

#define WOTLEN_FEATURES 1
#define WOTLEN_MEM_SEGS 1
#define WOTLEN (8 + 3 * WOTLEN_FEATURES + 8 * WOTLEN_MEM_SEGS)

/**
 * @brief
 */
uchar wotrustring[] = {
    WOTLEN - 1,  // Length of rest of record HERE
    (WOTLEN - 3) & 0xFF,
    ((WOTLEN - 3) >> 8) & 0xFF,  // Length of rest of message (H)
    1,
    0,
    0,                // Processor type (B, H)
    WOTLEN_FEATURES,  // Feature count (B)
    0,
    9,
    0,                // Feature ID (B, H)
    WOTLEN_MEM_SEGS,  // Memory segment count (B)
    0x00,
    0x00,
    0x00,
    0x00,  // Memory segment address (W)
    mem_size & 0xFF,
    (mem_size >> 8) & 0xFF,  // Memory segment
    (mem_size >> 16) & 0xFF,
    (mem_size >> 24) & 0xFF};  //  length (W)

BreakElement breakpoints[NO_OF_BREAKPOINTS];
BreakElement watchpoints[NO_OF_WATCHPOINTS];

uint emul_bp_flag[2];
uint emul_wp_flag[2];

uchar memory[RAMSIZE];

uchar status, old_status;
int steps_togo;    // Number of left steps before halting (0 is infinite)
uint steps_reset;  // Number of steps since last reset
char runflags;
uchar rtf;
bool breakpoint_enable;   // Breakpoints will be checked
bool breakpoint_enabled;  // Breakpoints will be checked now
bool run_through_BL;      // Treat BL as a single step
bool run_through_SWI;     // Treat SWI as a single step

uint tube_address;

int r[16];
int fiq_r[7];
int irq_r[2];
int sup_r[2];
int abt_r[2];
int undef_r[2];
uint cpsr;
uint spsr[32];  // Lots of wasted space - safe for any "mode"

bool print_out;
int run_until_PC, run_until_SP, run_until_mode;  // Used to determine when
uchar run_until_status;  //  to finish a `stepped' subroutine, SWI, etc.

uint exception_para[9];

int next_file_handle;
FILE*(file_handle[20]);

int count;

uint last_addr;

int glob1, glob2;

int past_opc_addr[32];  // History buffer of fetched op. code addresses
int past_size;          // Used size of buffer
int past_opc_ptr;       // Pointer into same
int past_count;         // Count of hits in instruction history

// Thumb stuff
int PC;
int BL_prefix, BL_address;
int next_char;
int ARM_flag;

struct pollfd* SWI_poll;  // Pointer to allow SWIs to scan input - YUK!

ringBuffer terminal0_Tx, terminal0_Rx;
ringBuffer terminal1_Tx, terminal1_Rx;
ringBuffer* terminal_table[16][2];

/**
 * @brief Program entry point.
 * @return int Exit code.
 */
int main(int argc, char** argv) {
  {
    for (int i = 0; i < 16; i++) {
      terminal_table[i][0] = NULL;
      terminal_table[i][1] = NULL;
    }

    initBuffer(&terminal0_Tx);  // Initialise terminal
    initBuffer(&terminal0_Rx);
    terminal_table[0][0] = &terminal0_Tx;
    terminal_table[0][1] = &terminal0_Rx;
    initBuffer(&terminal1_Tx);  // Initialise terminal
    initBuffer(&terminal1_Rx);
    terminal_table[1][0] = &terminal1_Tx;
    terminal_table[1][1] = &terminal1_Rx;
  }

  pollfd.fd = 0;
  pollfd.events = POLLIN;
  SWI_poll = &pollfd;  // Grubby pass to "mySystem"

  emulsetup();

  emul_bp_flag[0] = 0;
  if (NO_OF_BREAKPOINTS == 0) {
    emul_bp_flag[1] = 0x00000000;  // C work around
  } else {
    emul_bp_flag[1] = (1 << NO_OF_WATCHPOINTS) - 1;
  }

  emul_wp_flag[0] = 0;
  if (NO_OF_WATCHPOINTS == 0) {
    emul_wp_flag[1] = 0x00000000;  // C work around
  } else {
    emul_wp_flag[1] = (1 << NO_OF_WATCHPOINTS) - 1;
  }

  while (true) {
    comm(&pollfd);  // Check for monitor command
    if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING) {
      step();  // Step emulator as required
    } else {
      poll(&pollfd, 1, -1);  // If not running, deschedule until command arrives
    }
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

void step() {
  old_status = status;
  executeInstruction();

  if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING) {
    /* Still running - i.e. no breakpoint (etc.) found */

    if (status == CLIENT_STATE_RUNNING_SWI)  // OR _BL
    { /* Don't count the instructions from now */
      if ((getRegisterMonitor(15, reg_current) == run_until_PC) &&
          (getRegisterMonitor(13, reg_current) == run_until_SP) &&
          ((getRegisterMonitor(16, reg_current) & 0x3F) == run_until_mode)) {
        status = run_until_status;
      }
    } /* This can have changed status - hence no "else" below */

    if (status != CLIENT_STATE_RUNNING_SWI)  // OR _BL
    {
      /* Count steps unless inside routine */
      steps_reset++;
      if (steps_togo > 0) /* Stepping */
      {
        steps_togo--; /* If -decremented- to reach zero, stop. */
        if (steps_togo == 0) {
          status = CLIENT_STATE_STOPPED;
        }
      }
    } /* Running a whole routine */
  }

  if ((status & CLIENT_STATE_CLASS_MASK) != CLIENT_STATE_CLASS_RUNNING) {
    breakpoint_enabled = false; /* No longer running - allow "continue" */
  }
}

void monitor_options_misc(uchar command) {
  uchar tempchar;
  int temp;
  switch (command & 0x3F) {
    case BR_NOP:
      break;
    case BR_PING:
      if (write(1, "OK00", 4) < 0) {
        std::cout << "Some error occured!" << std::endl;
      }
      break;
    case BR_WOT_R_U:
      sendCharArray(wotrustring[0], &wotrustring[1]);
      /* wotrustring[0] holds length, followed by message */
      break;

    case BR_RESET:
      boardreset();
      break;

    case BR_RTF_GET:
      sendChar(rtf);
      break;

    case BR_RTF_SET:
      getChar(&rtf);
      break;

    case BR_WOT_U_DO:
      sendChar(status);
      sendNBytes(steps_togo, 4);
      sendNBytes(steps_reset, 4);
      break;

    case BR_PAUSE:
    case BR_STOP:
      if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING) {
        old_status = status;
        status = CLIENT_STATE_STOPPED;
      }
      break;

    case BR_CONTINUE:
      if (((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_STOPPED) &&
          (status != CLIENT_STATE_BYPROG))  // Only act if already stopped
        if ((old_status = CLIENT_STATE_STEPPING) || (steps_togo != 0))
          status = old_status;
      break;

    case BR_BP_GET:
      sendNBytes(emul_bp_flag[0], 4);
      sendNBytes(emul_bp_flag[1], 4);
      break;

    case BR_BP_SET: {
      int data[2];
      getNBytes(&data[0], 4);
      getNBytes(&data[1], 4);
      /* Note ordering to avoid temporary variable */
      emul_bp_flag[1] =
          (~emul_bp_flag[0] & emul_bp_flag[1]) |
          (emul_bp_flag[0] & ((emul_bp_flag[1] & ~data[0]) | data[1]));
      emul_bp_flag[0] = emul_bp_flag[0] & (data[0] | ~data[1]);
    } break;

    case BR_BP_READ:
      getChar(&tempchar);
      temp = tempchar;
      sendChar(breakpoints[temp].cond);
      sendChar(breakpoints[temp].size);
      sendNBytes(breakpoints[temp].addra, 4);
      sendNBytes(breakpoints[temp].addrb, 4);
      sendNBytes(breakpoints[temp].dataa[0], 4);
      sendNBytes(breakpoints[temp].dataa[1], 4);
      sendNBytes(breakpoints[temp].datab[0], 4);
      sendNBytes(breakpoints[temp].datab[1], 4);
      break;

    case BR_BP_WRITE:
      getChar(&tempchar);
      temp = tempchar;
      getChar(&breakpoints[temp].cond);
      getChar(&breakpoints[temp].size);
      getNBytes(&breakpoints[temp].addra, 4);
      getNBytes(&breakpoints[temp].addrb, 4);
      getNBytes(&breakpoints[temp].dataa[0], 4);
      getNBytes(&breakpoints[temp].dataa[1], 4);
      getNBytes(&breakpoints[temp].datab[0], 4);
      getNBytes(&breakpoints[temp].datab[1], 4);
      /* add breakpoint */
      temp = (1 << temp) & ~emul_bp_flag[0];
      emul_bp_flag[0] |= temp;
      emul_bp_flag[1] |= temp;
      break;

    case BR_WP_GET:
      sendNBytes(emul_wp_flag[0], 4);
      sendNBytes(emul_wp_flag[1], 4);
      break;

    case BR_WP_SET: {
      int data[2];
      getNBytes(&data[0], 4);
      getNBytes(&data[1], 4);
      temp = data[1] & ~data[0];
      emul_wp_flag[0] &= ~temp;
      emul_wp_flag[1] |= temp;
      temp = data[0] & emul_wp_flag[0];
      emul_wp_flag[1] = (emul_wp_flag[1] & ~temp) | (data[1] & temp);
    } break;

    case BR_WP_READ:
      getChar(&tempchar);
      temp = tempchar;
      sendChar(watchpoints[temp].cond);
      sendChar(watchpoints[temp].size);
      sendNBytes(watchpoints[temp].addra, 4);
      sendNBytes(watchpoints[temp].addrb, 4);
      sendNBytes(watchpoints[temp].dataa[0], 4);
      sendNBytes(watchpoints[temp].dataa[1], 4);
      sendNBytes(watchpoints[temp].datab[0], 4);
      sendNBytes(watchpoints[temp].datab[1], 4);
      break;

    case BR_WP_WRITE:
      getChar(&tempchar);
      temp = tempchar;
      getChar(&watchpoints[temp].cond);
      getChar(&watchpoints[temp].size);
      getNBytes(&watchpoints[temp].addra, 4);
      getNBytes(&watchpoints[temp].addrb, 4);
      getNBytes(&watchpoints[temp].dataa[0], 4);
      getNBytes(&watchpoints[temp].dataa[1], 4);
      getNBytes(&watchpoints[temp].datab[0], 4);
      getNBytes(&watchpoints[temp].datab[1], 4);
      temp = 1 << temp & ~emul_wp_flag[0];
      emul_wp_flag[0] |= temp;
      emul_wp_flag[1] |= temp;
      break;

    case BR_FR_WRITE: {
      uchar device, length;
      ringBuffer* pBuff;

      getChar(&device);
      pBuff = terminal_table[device][1];
      getChar(&length);
      temp = tempchar;
      while (length-- > 0) {
        getChar(&tempchar); /* Read character */
        if (pBuff != NULL)
          putBuffer(pBuff, tempchar); /*  and put in buffer */
      }
      sendChar(0);
    } break;

    case BR_FR_READ: {
      unsigned char device, max_length;
      uint i, length, available;
      ringBuffer* pBuff;

      getChar(&device);
      pBuff = terminal_table[device][0];
      getChar(&max_length);
      available = countBuffer(&terminal0_Tx); /* See how many chars we have */
      if (pBuff == NULL)
        length = 0; /* Kill if no corresponding buffer */
      else
        length = MIN(available, max_length); /* else clip to message max. */
      sendChar(length);
      for (i = 0; i < length; i++) /* Send zero or more characters */
      {
        unsigned char c;
        getBuffer(pBuff, &c);
        sendChar(c);
      }
    } break;

    default:
      break;
  }
}

void monitor_memory(uchar c) {
  int addr;
  unsigned char* pointer;
  int size;

  getNBytes(&addr, 4);  // Start address really
  if ((c & 0x30) == 0x10) {
    int temp;
    int reg_bank, reg_number;

    switch (addr & 0xE0) {
      case 0x00:
        reg_bank = reg_current;
        break;
      case 0x20:
        reg_bank = reg_user;
        break;
      case 0x40:
        reg_bank = regSvc;
        break;
      case 0x60:
        reg_bank = regAbt;
        break;
      case 0x80:
        reg_bank = reg_undef;
        break;
      case 0xA0:
        reg_bank = regIrq;
        break;
      case 0xC0:
        reg_bank = regFiq;
        break;
      default:
        reg_bank = reg_current;
        break;
    }
    reg_number = addr & 0x1F;

    getNBytes(&size, 2); /* Length of transfer */

    while (size--)
      if ((c & 8) != 0)
        sendNBytes(getRegisterMonitor(reg_number++, reg_bank), 4);
      else {
        getNBytes(&temp, 4);
        putRegister(reg_number++, temp, reg_bank);
      }
  } else {
    pointer = memory + (addr & (RAMSIZE - 1));
    getNBytes(&size, 2);
    size *= 1 << (c & 7);
    if (((uchar*)pointer + size) > ((uchar*)memory + RAMSIZE))
      pointer -= RAMSIZE;
    if (c & 8)
      sendCharArray(size, pointer);
    else
      getCharArray(size, pointer);
  }
}

void monitor_breakpoints(uchar c) {
  runflags = c & 0x3F;
  breakpoint_enable = (runflags & 0x10) != 0;
  breakpoint_enabled = (runflags & 0x01) != 0; /* Break straight away */
  run_through_BL = (runflags & 0x02) != 0;
  run_through_SWI = (runflags & 0x04) != 0;
  getNBytes(&steps_togo, 4);
  if (steps_togo == 0)
    status = CLIENT_STATE_RUNNING;
  else
    status = CLIENT_STATE_STEPPING;
}

void comm(struct pollfd* pPollfd) {
  uchar c;

  if (poll(pPollfd, 1, 0) > 0) {
    if (read(0, &c, 1) < 0) {
      std::cout << "Some error occured!" << std::endl;
    }  // Look at error return - find EOF & exit
    switch (c & 0xC0) {
      case 0x00:
        monitor_options_misc(c);
        break;
      case 0x40:
        monitor_memory(c);
        break;
      case 0x80:
        monitor_breakpoints(c);
        break;
      case 0xC0:
        break;
    }
  }
}

/**
 * @brief Get 1 character from host.
 * @param to_get
 * @return int
 */
int getChar(unsigned char* to_get) {
  return getCharArray(1, to_get);
}

/**
 * @brief Send 1 character to host
 * @param to_send
 * @return int
 */
int sendChar(unsigned char to_send) {
  return sendCharArray(1, &to_send);
}

/**
 * @brief Sends N bytes from the supplied value to the host (??), LSB first.
 * @param value
 * @param N
 * @return int The number of bytes believed received successfully (i.e. N=>"Ok")
 */
int sendNBytes(int value, int N) {
  unsigned char buffer[MAX_SERIAL_WORD];
  int i;

  if (N > MAX_SERIAL_WORD)
    N = MAX_SERIAL_WORD; /* Clip, just in case ... */

  for (i = 0; i < N; i++) {
    buffer[i] = value & 0xFF; /* Byte into buffer */
    value = value >> 8;       /* Get next byte */
  }

  return sendCharArray(N, buffer);
}

/**
 * @brief Gets N bytes from the host (??) into the indicated val_ptr, LSB first.
 * If error suspected sets `board_version' to not present
 * @param val_ptr
 * @param N
 * @return int The number of bytes received successfully (i.e. N=>"Ok")
 */
int getNBytes(int* val_ptr, int N) {
  unsigned char buffer[MAX_SERIAL_WORD];
  int i, No_received;

  if (N > MAX_SERIAL_WORD) {
    N = MAX_SERIAL_WORD;  // Clip, just in case ...
  }

  No_received = getCharArray(N, buffer);

  *val_ptr = 0;

  for (i = 0; i < No_received; i++) {
    *val_ptr =
        *val_ptr | ((buffer[i] & 0xFF) << (i * 8)); /* Assemble integer */
  }

  if (No_received != N) {
    board_version = -1;
  }

  return No_received;
}

/**
 * @brief Reads a character array from buffer. Sends char_number number of
 * characters given by data_ptr.
 * @param char_number
 * @param data_ptr
 * @return int Number of bytes received.
 */
int getCharArray(int char_number, unsigned char* data_ptr) {
  int ret = char_number;
  int replycount = 0;
  struct pollfd pollfd;

  pollfd.fd = 0;
  pollfd.events = POLLIN;

  while (char_number) {
    if (!poll(&pollfd, 1, -1)) {
      return ret - char_number;
    }

    replycount = read(0, data_ptr, char_number);
    if (replycount < 0) {
      replycount = 0;
    }

    char_number -= replycount;
    data_ptr += replycount;
  }

  return ret;
}

/**
 * @brief writes an array of bytes in the buffer.
 * @param char_number number of bytes given by data_ptr
 * @param data_ptr points to the beginning of the sequence to be sent
 * @return int
 */
int sendCharArray(int char_number, unsigned char* data_ptr) {
  if (write(1, data_ptr, char_number) < 0) {
    std::cout << "Some error occured!" << std::endl;
  }

  return char_number;  // send char array to the board
}

/**
 * @brief
 */
void emulsetup() {
  int initial_mode;

  glob1 = 0;
  glob2 = 0;

  {
    int i;

    for (i = 0; i < 32; i++) {
      past_opc_addr[i] = 1; /* Illegal op. code address */
    }
    past_opc_ptr = 0;
    past_count = 0;
    past_size = 4;
  }

  initial_mode = 0xC0 | supMode;
  print_out = false;

  next_file_handle = 1;

  initialise(0, initial_mode);
}

/**
 * @brief
 *
 * @param instr_addr
 * @param instr
 * @return true
 * @return false
 */
bool check_breakpoint(uint instr_addr, uint instr) {
  bool may_break = false;

  for (int i = 0; (i < NO_OF_BREAKPOINTS) && !may_break; i++) {
    may_break = ((emul_bp_flag[0] & emul_bp_flag[1] & (1 << i)) !=
                 0);  // Breakpoint is active

    // Try address comparison
    if (may_break) {
      switch (breakpoints[i].cond & 0x0C) {
        case 0x00:
        case 0x04:
          may_break = false;
          break;
        // Case of between address A and address B
        case 0x08:
          if ((instr_addr < breakpoints[i].addra) ||
              (instr_addr > breakpoints[i].addrb)) {
            may_break = false;
          }
          break;
        // case of mask
        case 0x0C:
          if ((instr_addr & breakpoints[i].addrb) != breakpoints[i].addra) {
            may_break = false;
          }
          break;
      }
    }

    // Try data comparison
    if (may_break) {
      switch (breakpoints[i].cond & 0x03) {
        case 0x00:
          may_break = false;
          break;

        case 0x01:
          may_break = false;
          break;

        case 0x02:  // Case of between data A and data B
          if ((instr < breakpoints[i].dataa[0]) ||
              (instr > breakpoints[i].datab[0])) {
            may_break = false;
          }
          break;

        case 0x03:  // Case of mask
          if ((instr & breakpoints[i].datab[0]) != breakpoints[i].dataa[0]) {
            may_break = false;
          }
          break;
      }
    }
  }
  return may_break;
}

void executeInstruction() {
  int i;

  uint instr_addr =
      getRegister(15, reg_current) - instructionLength(cpsr, tf_mask);
  last_addr = getRegister(15, reg_current) - instructionLength(cpsr, tf_mask);

  /* FETCH */
  auto instr = fetch();

  if ((breakpoint_enabled) && (status != CLIENT_STATE_RUNNING_SWI)) {
    if (check_breakpoint(instr_addr, instr)) {
      status = CLIENT_STATE_BREAKPOINT;
      return;
    }
  }
  breakpoint_enabled = breakpoint_enable; /* More likely after first fetch */

  /* BL instruction */
  if (((instr & 0x0F000000) == 0x0B000000) && run_through_BL) {
    saveState(CLIENT_STATE_RUNNING_BL);
  } else {
    if (((instr & 0x0F000000) == 0x0F000000) && run_through_SWI) {
      saveState(CLIENT_STATE_RUNNING_SWI);
    }
  }

  /* Execute */
  execute(instr);
}

/**
 * @brief Save state for leaving "procedure" {PC, SP, Mode, current state}
 * @param new_status
 */
void saveState(uchar new_status) {
  run_until_PC =
      getRegister(15, reg_current);  // Incremented once: correct here
  run_until_SP = getRegister(13, reg_current);
  run_until_mode = getRegister(16, reg_current) & 0x3F;  // Just the mode bits
  run_until_status = status;
  status = new_status;
}

void boardreset() {
  steps_reset = 0;
  initialise(0, supMode);
}

void initialise(uint start_address, int initial_mode) {
  cpsr = 0X000000C0 | initial_mode;  // Disable interrupts
  r[15] = start_address;
  old_status = CLIENT_STATE_RESET;
  status = CLIENT_STATE_RESET;
}

void execute(uint op_code) {
  incPC(); /* Easier here than later */

  /* ARM or THUMB ? */
  if ((cpsr & tf_mask) != 0) /* Thumb */
  {
    op_code = op_code & 0XFFFF; /* 16-bit op. code */
    switch (op_code & 0XE000) {
      case 0X0000:
        data0(op_code);
        break;
      case 0X2000:
        data1(op_code);
        break;
      case 0X4000:
        data_transfer(op_code);
        break;
      case 0X6000:
        transfer0(op_code);
        break;
      case 0X8000:
        transfer1(op_code);
        break;
      case 0XA000:
        sp_pc(op_code);
        break;
      case 0XC000:
        lsm_b(op_code);
        break;
      case 0XE000:
        thumb_branch(op_code);
        break;
    }
  } else {
    /* Check condition */
    if ((checkCC(op_code >> 28) == true) ||
        ((op_code & 0XFE000000) == 0XFA000000)) /* Nasty non-orthogonal BLX */
    {
      switch ((op_code >> 25) & 0X00000007) {
        case 0X0:
          dataOp(op_code);
          break; /* includes load/store hw & sb */
        case 0X1:
          dataOp(op_code);
          break; /* data processing & MSR # */
        case 0X2:
          transfer(op_code);
          break;
        case 0X3:
          transfer(op_code);
          break;
        case 0X4:
          multiple(op_code);
          break;
        case 0X5:
          branch(op_code);
          break;
        case 0X6:
          undefined();
          break;
        case 0X7:
          mySystem(op_code);
          break;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

int is_it_sbhw(uint op_code) {
  if (((op_code & 0X0E000090) == 0X00000090) &&
      ((op_code & 0X00000060) != 0X00000000)     /* No multiplies */
      && ((op_code & 0X00100040) != 0X00000040)) /* No signed stores */
  {
    if (((op_code & 0X00400000) != 0) || ((op_code & 0X00000F00) == 0))
      return true;
    else
      return false;
  } else
    return false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void dataOp(uint op_code) {
  int operation;

  if (((op_code & mul_mask) == mul_op) ||
      ((op_code & long_mul_mask) == long_mul_op)) {
    myMulti(op_code);
  } else if (is_it_sbhw(op_code) == true) {
    transfer_sbhw(op_code);
  } else if ((op_code & swp_mask) == swp_op) {
    swap(op_code);
  } else {
    operation = (op_code & data_op_mask) >> 21;

    /* TST, TEQ, CMP, CMN - all lie in following range, but have S set */
    if ((op_code & data_ext_mask) == arith_ext) /* PSR transfers OR BX */
    {
      if ((op_code & 0X0FBF0FFF) == 0X010F0000) {
        mrs(op_code); /* MRS */
      } else if (((op_code & 0X0DB6F000) == 0X0120F000) &&
                 ((op_code & 0X02000010) != 0X00000010)) {
        msr(op_code);                                  /* MSR */
      } else if ((op_code & 0X0FFFFFD0) == 0X012FFF10) /* BX/BLX */
      {
        bx(op_code & rm_mask, op_code & 0X00000020);
      } else if ((op_code & 0XFFF000F0) == 0XE1200070) {
        breakpoint(); /* Breakpoint */
      } else if ((op_code & 0X0FFF0FF0) == 0X016F0F10) {
        clz(op_code); /* CLZ */
      } else {
        undefined();
      }
    } else {
      normalDataOp(op_code, operation); /* All data processing operations */
    }
  }
}

/*----------------------------------------------------------------------------*/

void transfer_sbhw(uint op_code) {
  uint address;
  int size;
  int offset, rd;
  bool sign;

  switch (op_code & 0X00000060) {
    case 0X00:
      fprintf(stderr, "Multiply shouldn't be here!\n");
      break; /* Error! */
    case 0X20:
      size = 2;
      sign = false;
      break; /* H */
    case 0X40:
      size = 1;
      sign = true;
      break; /* SB */
    case 0X60:
      size = 2;
      sign = true;
      break; /* SH */
  }

  rd = ((op_code & rd_mask) >> 12);

  address = getRegister(((op_code & rn_mask) >> 16), reg_current);

  offset = transferOffset(op_code & op2_mask, op_code & up_mask,
                          op_code & imm_hw_mask, true);

  if ((op_code & pre_mask) != 0)
    address = address + offset; /* pre-index */

  if ((op_code & load_mask) == 0) /* store */
    writeMemory(address, getRegister(rd, reg_current), size, false, memData);
  else /* load */
    putRegister(rd, readMemory(address, size, sign, false, memData),
                reg_current);
  /* post index */

  if ((op_code & pre_mask) == 0) /* post index with writeback */
    putRegister((op_code & rn_mask) >> 16, address + offset, reg_current);
  else if ((op_code & write_back_mask) != 0)
    putRegister((op_code & rn_mask) >> 16, address, reg_current);
}

/*----------------------------------------------------------------------------*/

void mrs(uint op_code) {
  if ((op_code & 0X00400000) == 0)
    putRegister((op_code & rd_mask) >> 12, cpsr, reg_current);
  else
    putRegister((op_code & rd_mask) >> 12, spsr[cpsr & mode_mask], reg_current);
}

/*----------------------------------------------------------------------------*/

void msr(uint op_code) {
  int mask, source;

  switch (op_code & 0X00090000) {
    case 0X00000000:
      mask = 0X00000000;
      break;
    case 0X00010000:
      mask = 0X0FFFFFFF;
      break;
    case 0X00080000:
      mask = 0XF0000000;
      break;
    case 0X00090000:
      mask = 0XFFFFFFFF;
      break;
  }
  if ((cpsr & mode_mask) == 0X10)
    mask = mask & 0XF0000000; /* User mode */

  if ((op_code & imm_mask) == 0) /* Test applies for both cases */
    source = getRegister(op_code & rm_mask, reg_current) & mask;
  else {
    uint x, y;
    int dummy;

    x = op_code & 0X0FF;        /* Immediate value */
    y = (op_code & 0XF00) >> 7; /* Number of rotates */
    source = ((x >> y) | lsl(x, 32 - y, &dummy)) & mask;
  }

  if ((op_code & 0X00400000) == 0)
    cpsr = (cpsr & ~mask) | source;
  else
    spsr[cpsr & mode_mask] = (spsr[cpsr & mode_mask] & ~mask) | source;
}

/*----------------------------------------------------------------------------*/

void bx(uint Rm, int link) /* Link is performed if "link" is NON-ZERO */
{
  int PC, offset;
  int t_bit;

  PC = getRegister(15, reg_current);

  if ((cpsr & tf_mask) != 0) {
    PC = PC - 2;
    PC = PC | 1;
  } /* Remember Thumb mode */
  else {
    PC = PC - 4;
  }

  offset = getRegister(Rm, reg_current) & 0XFFFFFFFE;
  t_bit = getRegister(Rm, reg_current) & 0X00000001;

  if (t_bit == 1)
    cpsr = cpsr | tf_mask;
  else
    cpsr = cpsr & ~tf_mask;

  putRegister(15, offset, reg_current); /* Update PC */

  if (link != 0) {
    putRegister(14, PC, reg_current); /* Link if BLX */
  }
}

/*----------------------------------------------------------------------------*/

void myMulti(uint op_code) {
  int acc;

  if ((op_code & mul_long_bit) == 0) /* Normal */
  {
    acc = getRegister(op_code & rm_mask, reg_current) *
          getRegister((op_code & rs_mask) >> 8, reg_current);

    if ((op_code & mul_acc_bit) != 0)
      acc = acc + getRegister((op_code & rd_mask) >> 12, reg_current);

    putRegister((op_code & rn_mask) >> 16, acc, reg_current);

    if ((op_code & s_mask) != 0)
      setNZ(acc); /* Flags */
  } else          /* Long */
  {
    uint Rm, Rs, th, tm, tl;
    int sign;

    Rm = getRegister(op_code & rm_mask, reg_current);
    Rs = getRegister((op_code & rs_mask) >> 8, reg_current);

    sign = 0;
    if ((op_code & mul_sign_bit) != 0) /* Signed */
    {
      if ((Rm & bit_31) != 0) {
        Rm = ~Rm + 1;
        sign = 1;
      }
      if ((Rs & bit_31) != 0) {
        Rs = ~Rs + 1;
        sign = sign ^ 1;
      }
    }
    /* Everything now `positive' */
    tl = (Rm & 0X0000FFFF) * (Rs & 0X0000FFFF);
    th = ((Rm >> 16) & 0X0000FFFF) * ((Rs >> 16) & 0X0000FFFF);
    tm = ((Rm >> 16) & 0X0000FFFF) * (Rs & 0X0000FFFF);
    Rm =
        ((Rs >> 16) & 0X0000FFFF) * (Rm & 0X0000FFFF); /* Rm no longer needed */
    tm = tm + Rm;
    if (tm < Rm)
      th = th + 0X00010000; /* Propagate carry */
    tl = tl + (tm << 16);
    if (tl < (tm << 16))
      th = th + 1;
    th = th + ((tm >> 16) & 0X0000FFFF);

    if (sign != 0) /* Change sign of result */
    {
      th = ~th;
      tl = ~tl + 1;
      if (tl == 0)
        th = th + 1;
    }

    if ((op_code & mul_acc_bit) != 0) {
      tm = tl + getRegister((op_code & rd_mask) >> 12, reg_current);
      if (tm < tl)
        th = th + 1; /* Propagate carry */
      tl = tm;
      th = th + getRegister((op_code & rn_mask) >> 16, reg_current);
    }

    putRegister((op_code & rd_mask) >> 12, tl, reg_current);
    putRegister((op_code & rn_mask) >> 16, th, reg_current);

    if ((op_code & s_mask) != 0)
      setNZ(th | (((tl >> 16) | tl) & 0X0000FFFF)); /* Flags */
  }
}

/*----------------------------------------------------------------------------*/

void swap(uint op_code) {
  uint address, data, size;

  address = getRegister((op_code & rn_mask) >> 16, reg_current);

  if ((op_code & byte_mask) != 0)
    size = 1;
  else
    size = 4;

  data = readMemory(address, size, false, false, memData);
  writeMemory(address, getRegister(op_code & rm_mask, reg_current), size, false,
              memData);
  putRegister((op_code & rd_mask) >> 12, data, reg_current);
}

/*----------------------------------------------------------------------------*/

void normalDataOp(uint op_code, int operation) {
  int rd, a, b, mode;
  int shift_carry;
  int CPSR_special;

  mode = cpsr & mode_mask;
  CPSR_special = false;
  shift_carry = 0;
  a = getRegister((op_code & rn_mask) >> 16,
                  reg_current); /* force_user = false */

  if ((op_code & imm_mask) == 0)
    b = bReg(op_code & op2_mask, &shift_carry);
  else
    b = bImmediate(op_code & op2_mask, &shift_carry);

  switch (operation) /* R15s @@?! */
  {
    case 0X0:
      rd = a & b;
      break; /* AND */
    case 0X1:
      rd = a ^ b;
      break; /* EOR */
    case 0X2:
      rd = a - b;
      break; /* SUB */
    case 0X3:
      rd = b - a;
      break; /* RSB */
    case 0X4:
      rd = a + b;
      break; /* ADD */
    case 0X5:
      rd = a + b;
      if ((cpsr & cf_mask) != 0)
        rd = rd + 1;
      break; /* ADC */
    case 0X6:
      rd = a - b - 1;
      if ((cpsr & cf_mask) != 0)
        rd = rd + 1;
      break; /* SBC */
    case 0X7:
      rd = b - a - 1;
      if ((cpsr & cf_mask) != 0)
        rd = rd + 1;
      break; /* RSC */
    case 0X8:
      rd = a & b;
      break; /* TST */
    case 0X9:
      rd = a ^ b;                        /* TEQ */
      if ((op_code & rd_mask) == 0XF000) /* TEQP */
      {
        CPSR_special = true;
        if (mode != userMode)
          cpsr = spsr[mode];
      }
      break;
    case 0XA:
      rd = a - b;
      break; /* CMP */
    case 0XB:
      rd = a + b;
      break; /* CMN */
    case 0XC:
      rd = a | b;
      break; /* ORR */
    case 0XD:
      rd = b;
      break; /* MOV */
    case 0XE:
      rd = a & ~b;
      break; /* BIC */
    case 0XF:
      rd = ~b;
      break; /* MVN */
  }

  if ((operation & 0XC) != 0X8) /* Return result unless a compare */
    putRegister((op_code & rd_mask) >> 12, rd, reg_current);

  if (((op_code & s_mask) != 0) && (CPSR_special != true)) /* S-bit */
  {                                         /* Want to change CPSR */
    if (((op_code & rd_mask) >> 12) == 0XF) /* PC and S-bit */
    {
      if (mode != userMode)
        cpsr = spsr[mode]; /* restore saved CPSR */
      else
        fprintf(stderr, "SPSR_user read attempted\n");
    } else /* other dest. registers */
    {
      switch (operation) { /* LOGICALs */
        case 0X0:          /* AND */
        case 0X1:          /* EOR */
        case 0X8:          /* TST */
        case 0X9:          /* TEQ */
        case 0XC:          /* ORR */
        case 0XD:          /* MOV */
        case 0XE:          /* BIC */
        case 0XF:          /* MVN */
          setNZ(rd);
          if (shift_carry == true)
            cpsr = cpsr | cf_mask; /* CF := output */
          else
            cpsr = cpsr & ~cf_mask; /* from shifter */
          break;

        case 0X2: /* SUB */
        case 0XA: /* CMP */
          setFlags(flagSub, a, b, rd, 1);
          break;

        case 0X6: /* SBC - Needs testing more !!!   @@@@ */
          setFlags(flagSub, a, b, rd, cpsr & cf_mask);
          break;

        case 0X3: /* RSB */
          setFlags(flagSub, b, a, rd, 1);
          break;

        case 0X7: /* RSC */
          setFlags(flagSub, b, a, rd, cpsr & cf_mask);
          break;

        case 0X4: /* ADD */
        case 0XB: /* CMN */
          setFlags(flagAdd, a, b, rd, 0);
          break;

        case 0X5: /* ADC */
          setFlags(flagAdd, a, b, rd, cpsr & cf_mask);
          break;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/
/* shift type: 00 = LSL, 01 = LSR, 10 = ASR, 11 = ROR                         */

int bReg(int op2, int* cf) {
  uint shift_type, reg, distance, result;
  reg = getRegister(op2 & 0X00F, reg_current); /* Register */
  shift_type = (op2 & 0X060) >> 5;             /* Type of shift */
  if ((op2 & 0X010) == 0) {                    /* Immediate value */
    distance = (op2 & 0XF80) >> 7;
    if (distance == 0) /* Special cases */
    {
      if (shift_type == 3) {
        shift_type = 4; /* RRX */
        distance = 1;   /* Something non-zero */
      } else if (shift_type != 0)
        distance = 32; /* LSL excluded */
    }
  } else
    distance = (getRegister((op2 & 0XF00) >> 8, reg_current) & 0XFF);
  /* Register value */

  *cf = ((cpsr & cf_mask) != 0); /* Previous carry */
  switch (shift_type) {
    case 0X0:
      result = lsl(reg, distance, cf);
      break; /* LSL */
    case 0X1:
      result = lsr(reg, distance, cf);
      break; /* LSR */
    case 0X2:
      result = asr(reg, distance, cf);
      break; /* ASR */
    case 0X3:
      result = ror(reg, distance, cf);
      break;  /* ROR */
    case 0X4: /* RRX #1 */
      result = reg >> 1;
      if ((cpsr & cf_mask) == 0)
        result = result & ~bit_31;
      else
        result = result | bit_31;
      *cf = ((reg & bit_0) != 0);
      break;
  }

  if (*cf)
    *cf = true;
  else
    *cf = false; /* Change to "bool" */
  return result;
}

/*----------------------------------------------------------------------------*/

int bImmediate(int op2, int* cf) {
  uint x, y;
  int dummy;

  x = op2 & 0X0FF;        /* Immediate value */
  y = (op2 & 0XF00) >> 7; /* Number of rotates */
  if (y == 0)
    *cf = ((cpsr & cf_mask) != 0); /* Previous carry */
  else
    *cf = (((x >> (y - 1)) & bit_0) != 0);
  if (*cf)
    *cf = true;
  else
    *cf = false;            /* Change to "bool" */
  return ror(x, y, &dummy); /* Circular rotation */
}

/*----------------------------------------------------------------------------*/

void clz(uint op_code) {
  int i, j;

  j = getRegister(op_code & rm_mask, reg_current);

  if (j == 0)
    i = 32;
  else {
    i = 0;
    while ((j & 0X80000000) == 0) {
      i++;
      j = j << 1;
    }
  }

  putRegister((op_code & rd_mask) >> 12, i, reg_current);
}

/*----------------------------------------------------------------------------*/

void transfer(uint op_code) {
  uint address;
  int offset, rd, size;
  bool T;

  if ((op_code & undef_mask) == undef_code) {
    undefined();
  } else {
    if ((op_code & byte_mask) == 0) {
      size = 4;
    } else {
      size = 1;
    }

    T = (((op_code & pre_mask) == 0) && ((op_code & write_back_mask) != 0));
    rd = (op_code & rd_mask) >> 12;
    address = getRegister((op_code & rn_mask) >> 16, reg_current);
    offset = transferOffset(op_code & op2_mask, op_code & up_mask,
                            op_code & imm_mask, false);

    if ((op_code & pre_mask) != 0) {
      address = address + offset;  // Pre-index
    }

    if ((op_code & load_mask) == 0) {
      writeMemory(address, getRegister(rd, reg_current), size, T, memData);
    } else {
      putRegister(rd, readMemory(address, size, false, T, memData),
                  reg_current);
    }

    // Post-index
    if ((op_code & pre_mask) == 0) {
      putRegister((op_code & rn_mask) >> 16, address + offset, reg_current);
    } else if ((op_code & write_back_mask) != 0) {
      putRegister((op_code & rn_mask) >> 16, address, reg_current);
    }
  }
}

/**
 * @brief Add and imm are zero/non-zero Booleans.
 * @param op2
 * @param add
 * @param imm
 * @param sbhw
 * @return int
 */
int transferOffset(int op2, int add, int imm, bool sbhw) {
  int offset;
  int cf;  // Dummy parameter

  // Addressing mode 2
  if (!sbhw) {
    if (imm != 0) {
      offset = bReg(op2, &cf);  // bit(25) = 1 -> reg
    } else {
      offset = op2 & 0XFFF;
    }
  }

  // Addressing mode 3
  else {
    if (imm != 0) {
      offset = ((op2 & 0XF00) >> 4) | (op2 & 0X00F);
    } else {
      offset = bReg(op2 & 0xF, &cf);
    }
  }

  if (add == 0) {
    offset = -offset;
  }

  return offset;
}

/**
 * @brief
 * @param op_code
 */
void multiple(uint op_code) {
  if ((op_code & load_mask) == 0) {
    stm((op_code & 0X01800000) >> 23, (op_code & rn_mask) >> 16,
        op_code & 0X0000FFFF, op_code & write_back_mask, op_code & user_mask);
  } else {
    ldm((op_code & 0X01800000) >> 23, (op_code & rn_mask) >> 16,
        op_code & 0X0000FFFF, op_code & write_back_mask, op_code & user_mask);
  }
}

/**
 * @brief
 * @param source
 * @param first
 * @return int
 */
int bitCount(uint source, int* first) {
  int count = 0;
  int reg = 0;
  *first = -1;

  while (source != 0) {
    if ((source & bit_0) != 0) {
      count = count + 1;
      if (*first < 0) {
        *first = reg;
      }
    }

    source = source >> 1;
    reg = reg + 1;
  }

  return count;
}

/**
 * @brief
 *
 * @param mode
 * @param Rn
 * @param reg_list
 * @param write_back
 * @param hat
 */
void ldm(int mode, int Rn, int reg_list, bool write_back, bool hat) {
  int address, new_base, count, first_reg, reg, data;
  int force_user;
  bool r15_inc;  // internal `bool'

  address = getRegister(Rn, reg_current);
  count = bitCount(reg_list, &first_reg);
  r15_inc = (reg_list & 0X00008000) != 0;  // R15 in list

  switch (mode) {
    case 0:
      new_base = address - 4 * count;
      address = new_base + 4;
      break;
    case 1:
      new_base = address + 4 * count;
      break;
    case 2:
      new_base = address - 4 * count;
      address = new_base;
      break;
    case 3:
      new_base = address + 4 * count;
      address = address + 4;
      break;
  }

  address = address & 0XFFFFFFFC;  // Bottom 2 bits ignored in address

  if (write_back) {
    putRegister(Rn, new_base, reg_current);
  }

  // Force user unless R15 in list
  if (hat && !r15_inc) {
    force_user = reg_user;
  } else {
    force_user = reg_current;
  }

  reg = 0;

  while (reg_list != 0) {
    if ((reg_list & bit_0) != 0) {
      data = readMemory(address, 4, false, false, memData);  // Keep for later
      putRegister(reg, data, force_user);
      address = address + 4;
    }

    reg_list = reg_list >> 1;
    reg = reg + 1;
  }

  // R15 in list
  if (r15_inc) {
    if ((data & 1) != 0) {
      cpsr = cpsr | tf_mask;  // data left over from last load
    } else {
      cpsr = cpsr & ~tf_mask;  // used to set instruction set
    }

    if (hat) {
      cpsr = spsr[cpsr & mode_mask];  // and if S bit set
    }
  }
}

/**
 * @brief
 * @param mode
 * @param Rn
 * @param reg_list
 * @param write_back
 * @param hat
 */
void stm(int mode, int Rn, int reg_list, bool write_back, bool hat) {
  int address, new_base, count, first_reg, reg;
  int force_user;
  bool special;

  address = getRegister(Rn, reg_current);
  count = bitCount(reg_list, &first_reg);

  switch (mode) {
    case 0:
      new_base = address - 4 * count;
      address = new_base + 4;
      break;
    case 1:
      new_base = address + 4 * count;
      break;
    case 2:
      new_base = address - 4 * count;
      address = new_base;
      break;
    case 3:
      new_base = address + 4 * count;
      address = address + 4;
      break;
  }

  address = address & 0XFFFFFFFC;  // Bottom 2 bits ignored in address

  special = false;
  if (write_back != 0) {
    if (Rn == first_reg) {
      special = true;
    } else {
      putRegister(Rn, new_base, reg_current);
    }
  }

  if (hat != 0) {
    force_user = reg_user;
  } else {
    force_user = reg_current;
  }

  reg = 0;

  while (reg_list != 0) {
    if ((reg_list & bit_0) != 0) {
      writeMemory(address, getRegister(reg, force_user), 4, false, memData);
      address = address + 4;
    }

    reg_list = reg_list >> 1;
    reg = reg + 1;
  }

  if (special)
    putRegister(Rn, new_base, reg_current);
}

/**
 * @brief
 * @param opCode
 */
void branch(uint opCode) {
  int PC = getRegister(15, reg_current);  // Get this now in case mode changes

  if (((opCode & link_mask) != 0) || ((opCode & 0XF0000000) == 0XF0000000)) {
    putRegister(14, getRegister(15, reg_current) - 4, reg_current);
  }

  int offset = (opCode & branch_field) << 2;

  if ((opCode & branch_sign) != 0) {
    offset = offset | (~(branch_field << 2) & 0XFFFFFFFC);  // sign extend
  }

  // Other BLX fix-up
  if ((opCode & 0XF0000000) == 0XF0000000) {
    offset = offset | ((opCode >> 23) & 2);
    cpsr = cpsr | tf_mask;
  }

  putRegister(15, PC + offset, reg_current);
}

/**
 * @brief
 * @param c
 * @return true
 * @return false
 */
bool swiCharacterPrint(char c) {
  while (!putBuffer(&terminal0_Tx, c)) {
    if (status == CLIENT_STATE_RESET) {
      return false;
    } else {
      comm(SWI_poll);  // If stalled, retain monitor communications
    }
  }

  return true;
}

/**
 * @brief Recursive zero suppression
 * @param number
 * @return int
 */
int swiDecimalPrint(uint number) {
  int okay;

  okay = true;
  if (number > 0) {
    okay = swiDecimalPrint(number / 10);  // Recursive call

    if (okay) {
      swiCharacterPrint((number % 10) | '0');  // Returns if reset
    }
  }
  return okay;
}

void mySystem(uint op_code) {
  int temp;

  if (((op_code & 0X0F000000) == 0X0E000000)
      /* bodge to allow Thumb to use this code */
      || ((op_code & 0X0F000000) == 0X0C000000) ||
      ((op_code & 0X0F000000) == 0X0D000000)) { /* Coprocessor op.s */
    fprintf(stderr, "whoops -undefined \n");
    undefined();
  } else {
    if (print_out) {
      fprintf(stderr, "\n*** SWI CALL %06X ***\n\n", op_code & 0X00FFFFFF);
    }

    switch (op_code & 0X00FFFFFF) {
      // Output character R0 (to terminal)
      case 0:
        putRegister(15, getRegister(15, reg_current) - 8, reg_current);
        swiCharacterPrint(getRegister(0, reg_current) & 0XFF);

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, reg_current),
                      reg_current);  // Correct PC
        }
        break;

      // Input character R0 (from terminal)
      case 1: {
        unsigned char c;
        putRegister(15, getRegister(15, reg_current) - 8, reg_current);
        // Bodge PC so that stall looks `correct'
        while ((!getBuffer(&terminal0_Rx, &c)) &&
               (status != CLIENT_STATE_RESET)) {
          comm(SWI_poll);
        }

        if (status != CLIENT_STATE_RESET) {
          putRegister(0, c & 0XFF, reg_current);
          putRegister(15, getRegister(15, reg_current),
                      reg_current);  // Correct PC
        }
      } break;

      // Halt
      case 2:
        status = CLIENT_STATE_BYPROG;
        break;

      // Print string @R0 (to terminal)
      case 3: {
        putRegister(15, getRegister(15, reg_current) - 8, reg_current);
        uint str_ptr = getRegister(0, reg_current);

        char c;
        while (
            ((c = readMemory(str_ptr, 1, false, false, memSystem)) != '\0') &&
            (status != CLIENT_STATE_RESET)) {
          swiCharacterPrint(c);  // Returns if reset
          str_ptr++;
        }

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, reg_current),
                      reg_current);  // Correct PC
        }
      } break;

      // Decimal print R0
      case 4: {
        putRegister(15, getRegister(15, reg_current) - 8, reg_current);
        const uint number = getRegister(0, reg_current);
        number == 0 ? swiCharacterPrint('0') : swiDecimalPrint(number);

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, reg_current),
                      reg_current);  // Correct PC
        }
      } break;

      default:
        if (print_out) {
          fprintf(stderr, "Untrapped SWI call %06X\n", op_code & 0X00FFFFFF);
        }

        spsr[supMode] = cpsr;
        cpsr = (cpsr & ~mode_mask) | supMode;
        cpsr = cpsr & ~tf_mask;  // Always in ARM mode
        putRegister(14, getRegister(15, reg_current) - 4, reg_current);
        putRegister(15, 8, reg_current);
        break;
    }
  }
}

/**
 * @brief This is the breakpoint instruction.
 */
void breakpoint() {
  spsr[abtMode] = cpsr;
  cpsr = (cpsr & ~mode_mask & ~tf_mask) | abtMode;
  putRegister(14, getRegister(15, reg_current) - 4, reg_current);
  putRegister(15, 12, reg_current);
}

/**
 * @brief
 */
void undefined() {
  spsr[undefMode] = cpsr;
  cpsr = (cpsr & ~mode_mask & ~tf_mask) | undefMode;
  putRegister(14, getRegister(15, reg_current) - 4, reg_current);
  putRegister(15, 4, reg_current);
}

/**
 * @brief
 * @param operation
 * @param a
 * @param b
 * @param rd
 * @param carry
 */
void setFlags(int operation, int a, int b, int rd, int carry) {
  setNZ(rd);
  setCF(a, rd, carry);
  switch (operation) {
    case 1:
      setVF_ADD(a, b, rd);
      break;
    case 2:
      setVF_SUB(a, b, rd);
      break;
    default:
      fprintf(stderr, "Flag setting error\n");
      break;
  }
}

/**
 * @brief
 * @param value
 */
void setNZ(uint value) {
  if (value == 0) {
    cpsr = cpsr | zf_mask;
  } else {
    cpsr = cpsr & ~zf_mask;
  }

  if ((value & bit_31) != 0) {
    cpsr = cpsr | nf_mask;
  } else {
    cpsr = cpsr & ~nf_mask;
  }
}

/**
 * @brief Two ways result can equal an operand.
 * @param a
 * @param rd
 * @param carry
 */
void setCF(uint a, uint rd, int carry) {
  if ((rd > a) || ((rd == a) && (carry == 0)))
    cpsr = cpsr & ~cf_mask;
  else
    cpsr = cpsr | cf_mask;
}

/**
 * @brief
 * @param a
 * @param b
 * @param rd
 */
void setVF_ADD(int a, int b, int rd) {
  cpsr = cpsr & ~vf_mask;  // Clear VF
  if (((~(a ^ b) & (a ^ rd)) & bit_31) != 0) {
    cpsr = cpsr | vf_mask;
  }
}

/**
 * @brief
 * @param a
 * @param b
 * @param rd
 */
void setVF_SUB(int a, int b, int rd) {
  cpsr = cpsr & ~vf_mask;  // Clear VF
  if ((((a ^ b) & (a ^ rd)) & bit_31) != 0) {
    cpsr = cpsr | vf_mask;
  }
}

/**
 * @brief checks CC against flag status
 * @param condition
 * @return true
 * @return false
 */
bool checkCC(int condition) {
  switch (condition & 0XF) {
    case 0X0:
      return zf(cpsr);
    case 0X1:
      return not zf(cpsr);
    case 0X2:
      return cf(cpsr);
    case 0X3:
      return not cf(cpsr);
    case 0X4:
      return nf(cpsr);
    case 0X5:
      return not nf(cpsr);
    case 0X6:
      return vf(cpsr);
    case 0X7:
      return not vf(cpsr);
    case 0X8:
      return cf(cpsr) and not zf(cpsr);
    case 0X9:
      return (not cf(cpsr)) or zf(cpsr);
    case 0XA:
      return not(nf(cpsr) xor vf(cpsr));
    case 0XB:
      return nf(cpsr) xor vf(cpsr);
    case 0XC:
      return (not zf(cpsr)) and (not(nf(cpsr) xor vf(cpsr)));
    case 0XD:
      return zf(cpsr) or (nf(cpsr) xor vf(cpsr));
    case 0XE:
      return true;
    case 0XF:
      return false;
    default:
      return true;
  }
}

constexpr const bool zf(const int cpsr) {
  if ((zf_mask & cpsr) != 0) {
    return true;
  }

  return false;
}

constexpr const bool cf(const int cpsr) {
  if ((cf_mask & cpsr) != 0) {
    return true;
  }

  return false;
}

constexpr const bool nf(const int cpsr) {
  if ((nf_mask & cpsr) != 0) {
    return true;
  }

  return false;
}

constexpr const bool vf(const int cpsr) {
  if ((vf_mask & cpsr) != 0) {
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

int getRegister(int reg_no, int force_mode) {
  int mode, value;

  switch (force_mode) {
    case reg_current:
      mode = cpsr & mode_mask;
      break;
    case reg_user:
      mode = userMode;
      break;
    case regSvc:
      mode = supMode;
      break;
    case regFiq:
      mode = fiqMode;
      break;
    case regIrq:
      mode = irqMode;
      break;
    case regAbt:
      mode = abtMode;
      break;
    case reg_undef:
      mode = undefMode;
      break;
  }

  if (reg_no == 16) {
    value = cpsr;  // Trap for status registers
  } else if (reg_no == 17) {
    if ((mode == userMode) || (mode == systemMode)) {
      value = cpsr;
    } else {
      value = spsr[mode];
    }
  } else if (reg_no != 15) {
    switch (mode) {
      case userMode:
      case systemMode:
        value = r[reg_no];
        break;

      case fiqMode:
        if (reg_no < 8) {
          value = r[reg_no];

        } else {
          value = fiq_r[reg_no - 8];
        }
        break;

      case irqMode:
        if (reg_no < 13) {
          value = r[reg_no];
        } else {
          value = irq_r[reg_no - 13];
        }
        break;

      case supMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = sup_r[reg_no - 13];
        }
        break;

      case abtMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = abt_r[reg_no - 13];
        }
        break;

      case undefMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = undef_r[reg_no - 13];
        }
        break;
    }
  } else {
    value = r[15] + instructionLength(cpsr, tf_mask);
  }

  return value;
}

/*----------------------------------------------------------------------------*/
/* Modified "getRegister" to give unadulterated copy of PC */

int getRegisterMonitor(int reg_no, int force_mode) {
  if (reg_no != 15)
    return getRegister(reg_no, force_mode);
  else
    return r[15]; /* PC access */
}

/*----------------------------------------------------------------------------*/
/* Write to a specified processor register                                    */

void putRegister(int reg_no, int value, int force_mode) {
  int mode;

  switch (force_mode) {
    case reg_current:
      mode = cpsr & mode_mask;
      break;
    case reg_user:
      mode = userMode;
      break;
    case regSvc:
      mode = supMode;
      break;
    case regFiq:
      mode = fiqMode;
      break;
    case regIrq:
      mode = irqMode;
      break;
    case regAbt:
      mode = abtMode;
      break;
    case reg_undef:
      mode = undefMode;
      break;
  }

  if (reg_no == 16)
    cpsr = value; /* Trap for status registers */
  else if (reg_no == 17) {
    if ((mode == userMode) || (mode == systemMode))
      cpsr = value;
    else
      spsr[mode] = value;
  } else if (reg_no != 15) {
    switch (mode) {
      case userMode:
      case systemMode:
        r[reg_no] = value;
        break;

      case fiqMode:
        if (reg_no < 8)
          r[reg_no] = value;
        else
          fiq_r[reg_no - 8] = value;
        break;

      case irqMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          irq_r[reg_no - 13] = value;
        break;

      case supMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          sup_r[reg_no - 13] = value;
        break;

      case abtMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          abt_r[reg_no - 13] = value;
        break;

      case undefMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          undef_r[reg_no - 13] = value;
        break;
    }
  } else
    r[15] = value & 0XFFFFFFFE; /* Lose bottom bit, but NOT mode specific! */
}

/**
 * @brief Return the length, in bytes, of the currently expected instruction.
 * @return int 4 for ARM, 2 for Thumb.
 */
constexpr const int instructionLength(const int cpsr, const int tf_mask) {
  if ((cpsr & tf_mask) == 0) {
    return 4;

  } else {
    return 2;
  }
}

/**
 * @brief
 * @return uint
 */
uint fetch() {
  uint op_code = readMemory(
      (getRegister(15, reg_current) - instructionLength(cpsr, tf_mask)),
      instructionLength(cpsr, tf_mask), false, false, memInstruction);

  for (int i = 0; i < 32; i++) {
    if (past_opc_addr[i] ==
        getRegister(15, reg_current) - instructionLength(cpsr, tf_mask)) {
      past_count++;
      i = 32;  // bodged escape from loop
    }
  }

  past_opc_addr[past_opc_ptr++] =
      getRegister(15, reg_current) - instructionLength(cpsr, tf_mask);
  past_opc_ptr = past_opc_ptr % past_size;

  return op_code;
}

/**
 * @brief getRegister returns PC+4 for ARM & PC+2 for THUMB.
 */
void incPC() {
  putRegister(15, getRegister(15, reg_current), reg_current);
}

void endianSwap(const uint start, const uint end) {
  for (uint i = start; i < end; i++) {
    uint j = getmem32(i);
    setmem32(i, ((j >> 24) & 0X000000FF) | ((j >> 8) & 0X0000FF00) |
                    ((j << 8) & 0X00FF0000) | ((j << 24) & 0XFF000000));
  }
}

/**
 * @brief
 *
 * @param address
 * @param size
 * @param sign
 * @param T
 * @param source indicates type of read {memSystem, memInstruction, memData}
 * @return int
 */
int readMemory(uint address, int size, bool sign, bool T, int source) {
  int data, alignment;

  if (address < mem_size) {
    alignment = address & 0X00000003;
    data = getmem32(address >> 2);

    switch (address & 0X00000003) /* Apply rotation */
    {
      case 0:
        break; /* RR  0 */
      case 1:
        data = (data << 24) | ((data >> 8) & 0X00FFFFFF);
        break; /* RR  8 */
      case 2:
        data = (data << 16) | ((data >> 16) & 0X0000FFFF);
        break; /* RR 16 */
      case 3:
        data = (data << 8) | ((data >> 24) & 0X000000FF);
        break; /* RR 24 */
    }

    switch (size) {
      case 0:
        data = 0;
        break; /* A bit silly really */

      case 1: /* byte access */
        if ((sign) && ((data & 0X00000080) != 0))
          data = data | 0XFFFFFF00;
        else
          data = data & 0X000000FF;
        break;

      case 2: /* half-word access */
        if ((sign) && ((data & 0X00008000) != 0))
          data = data | 0XFFFF0000;
        else
          data = data & 0X0000FFFF;
        break;

      case 4:
        break; /* word access */

      default:
        fprintf(stderr, "Illegally sized memory read\n");
    }

    /* check watchpoints enabled */
    if ((runflags & 0x20) && (source == memData)) {
      if (checkWatchpoints(address, data, size, 1)) {
        status = CLIENT_STATE_WATCHPOINT;
      }
    }
  } else {
    data = 0X12345678;
    print_out = false;
  }

  return data;
}

/*----------------------------------------------------------------------------*/

void writeMemory(uint address, int data, int size, bool T, int source) {
  uint mask;

  if ((address == tube_address) &&
      (tube_address != 0)) /* Deal with Tube output */
  {
    unsigned char c;

    c = data & 0XFF;

    if (!print_out) {
      if ((c == 0X0A) || (c == 0X0D))
        fprintf(stderr, "\n");
      else if ((c < 0X20) || (c >= 0X7F))
        fprintf(stderr, "%02X", c);
      else
        fprintf(stderr, "%c", c);
    } else {
      fprintf(stderr, "Tube output byte = %02X (", c);
      if ((c < 0X20) || (c >= 0X7F))
        fprintf(stderr, ".)\n");
      else
        fprintf(stderr, "%c)\n", c);
    }
  } else {
    if ((address >> 2) < mem_size) {
      switch (size) {
        case 0:
          break; /* A bit silly really */

        case 1: /* byte access */
          mask = 0X000000FF << (8 * (address & 0X00000003));
          data = data << (8 * (address & 0X00000003));
          setmem32(address >> 2,
                   (getmem32(address >> 2) & ~mask) | (data & mask));
          break;

        case 2: /* half-word acccess */
          mask = 0X0000FFFF << (8 * (address & 0X00000002));
          data = data << (8 * (address & 0X00000002));
          setmem32(address >> 2,
                   (getmem32(address >> 2) & ~mask) | (data & mask));
          break;

        case 4: /* word access */
          setmem32(address >> 2, data);
          break;

        default:
          fprintf(stderr, "Illegally sized memory write\n");
      }
    } else {
      // fprintf(stderr, "Writing %08X  data = %08X\n", address, data);
      print_out = false;
    }

    if ((runflags & 0x20) &&
        (source == memData)) /* check watchpoints enabled */
    {
      if (checkWatchpoints(address, data, size, 0)) {
        status = CLIENT_STATE_WATCHPOINT;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

/*- - - - - - - - - - - - watchpoints - - - - - - - - - - - - - - - - - - - */
/*                      to be completed                                     */

int checkWatchpoints(uint address, int data, int size, int direction) {
  bool may_break = false;

  for (int i = 0; (i < NO_OF_WATCHPOINTS) && !may_break; i++) {
    may_break = ((emul_wp_flag[0] & emul_wp_flag[1] & (1 << i)) != 0);
    /* Watchpoint is active */

    may_break &= ((watchpoints[i].size & size) != 0); /* Size is allowed? */

    if (may_break)
      if (direction == 0)
        may_break = (watchpoints[i].cond & 0x10) != 0;
      else
        may_break = (watchpoints[i].cond & 0x20) != 0;

    if (may_break) /* Try address comparison */
      switch (watchpoints[i].cond & 0x0C) {
        case 0x00:
          may_break = false;
          break;
        case 0x04:
          may_break = false;
          break;
        case 0x08: /* Case of between address A and address B */
          if ((address < watchpoints[i].addra) ||
              (address > watchpoints[i].addrb))
            may_break = false;
          break;

        case 0x0C: /* Case of mask */
          if ((address & watchpoints[i].addrb) != watchpoints[i].addra)
            may_break = false;
          break;
      }

    if (may_break) /* Try data comparison */
      switch (watchpoints[i].cond & 0x03) {
        case 0x00:
          may_break = false;
          break;
        case 0x01:
          may_break = false;
          break;
        case 0x02: /* Case of between data A and data B */
          if ((data < watchpoints[i].dataa[0]) ||
              (data > watchpoints[i].datab[0]))
            may_break = false;
          break;

        case 0x03: /* Case of mask */
          if ((data & watchpoints[i].datab[0]) != watchpoints[i].dataa[0])
            may_break = false;
          break;
      }
  }

  return may_break;
}

/*- - - - - - - - - - - - end watchpoints - - - - - - - - - - - - - - - - - */

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/* This is filthy :-( */
int getNumber(char* ptr) {
  int a;

  a = 0;
  while (*ptr == ' ')
    ptr++; /* Strip any leading spaces */

  while (((*ptr >= '0') && (*ptr <= '9')) || ((*ptr >= 'A') && (*ptr <= 'F'))) {
    if ((*ptr >= '0') && (*ptr <= '9'))
      a = 16 * a + *(ptr++) - '0';
    else
      a = 16 * a + *(ptr++) - 'A' + 10;
  }
  return a;
}

/*----------------------------------------------------------------------------*/
/* As the compiler can't manage it ...                                        */

int lsl(int value, int distance, int* cf) /* cf is -internal- bool */
{
  int result;

  if (distance != 0) {
    if (distance < 32) {
      result = value << distance;
      *cf = (((value << (distance - 1)) & bit_31) != 0);
    } else {
      result = 0X00000000;
      if (distance == 32)
        *cf = ((value & bit_0) != 0);
      else
        *cf = (0 != 0); /* internal "false" value */
    }
  } else
    result = value;

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int lsr(uint value, int distance, int* cf) /* cf is -internal- bool */
{
  uint result, mask;

  if (distance != 0) {
    if (distance < 32) {
      if (distance != 0)
        mask = ~(0XFFFFFFFF << (32 - distance));
      else
        mask = 0XFFFFFFFF;
      result = (value >> distance) & mask; /* Enforce logical shift */
      *cf = (((value >> (distance - 1)) & bit_0) != 0);
    } else { /* Make a special case because C is so crap */
      result = 0X00000000;
      if (distance == 32)
        *cf = ((value & bit_31) != 0);
      else
        *cf = (0 != 0); /* internal "false" value */
    }
  } else
    result = value;

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int asr(int value, int distance, int* cf) /* cf is -internal- bool */
{
  int result;

  if (distance != 0) {
    if (distance < 32) {
      result = value >> distance;
      if (((value & bit_31) != 0) && (distance != 0))
        result = result | (0XFFFFFFFF << (32 - distance));
      /* Sign extend - I don't trust the compiler */
      *cf = (((value >> (distance - 1)) & bit_0) != 0);
    } else { /* Make a special case because C is so crap */
      *cf = ((value & bit_31) != 0);
      if ((value & bit_31) == 0)
        result = 0X00000000;
      else
        result = 0XFFFFFFFF;
    }
  } else
    result = value;

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int ror(uint value, int distance, int* cf) /* cf is -internal- bool */
{
  int result;

  if (distance != 0) {
    distance = distance & 0X1F;
    result = lsr(value, distance, cf) | lsl(value, 32 - distance, cf);
    /* cf acts as dummy here */
    *cf = (((value >> (distance - 1)) & bit_0) != 0);
  } else
    result = value;

  return result;
}

/*----------------------------------------------------------------------------*/

/*------------------------------ THUMB EXECUTE -------------------------------*/

/*----------------------------------------------------------------------------*/

void data0(uint op_code) {
  uint op2, rn;
  uint shift, result;
  int cf;

  rn = getRegister(((op_code >> 3) & 7),
                   reg_current);         /* Called "Rm" in shifts */
  shift = ((op_code >> 6) & 0X0000001F); /* Extracted speculatively */

  if ((op_code & 0X1800) != 0X1800) /* Shifts */
  {
    cf = ((cpsr & cf_mask) != 0); /* default */
    switch (op_code & 0X1800) {
      case 0X0000:
        result = lsl(rn, shift, &cf);
        break;     /* LSL (1) */
      case 0X0800: /* LSR (1) */
        if (shift == 0)
          shift = 32;

        result = lsr(rn, shift, &cf);
        break;
      case 0X1000: /* ASR (1) */
        if (shift == 0)
          shift = 32;
        result = asr(rn, shift, &cf);
        break;
      default:
        fprintf(stderr, "This compiler is broken\n");
        break;
    }

    if (cf)
      cpsr = cpsr | cf_mask;
    else
      cpsr = cpsr & ~cf_mask;
    setNZ(result);
    putRegister((op_code & 7), result, reg_current);
  } else {
    if ((op_code & 0X0400) == 0) /* ADD(3)/SUB(3) */
    {
      op2 = getRegister((op_code >> 6) & 7, reg_current);
    } else /* ADD(1)/SUB(1) */
    {
      op2 = (op_code >> 6) & 7;
    };

    if ((op_code & 0X0200) == 0) {
      result = rn + op2;
      setFlags(flagAdd, rn, op2, result, 0);
    } else {
      result = rn - op2;
      setFlags(flagSub, rn, op2, result, 1);
    }

    putRegister(op_code & 7, result, reg_current);
  }
}

/*----------------------------------------------------------------------------*/

void data1(uint op_code) {
  int rd, imm;
  int result;

  rd = (op_code >> 8) & 7;
  imm = op_code & 0X00FF;

  switch (op_code & 0X1800) {
    case 0X0000: /* MOV (1) */
      result = imm;
      setNZ(result);
      putRegister(rd, result, reg_current);
      break;

    case 0X0800: /* CMP (1) */
      result = (getRegister(rd, reg_current) - imm);
      setFlags(flagSub, getRegister(rd, reg_current), imm, result, 1);
      break;

    case 0X1000: /* ADD (2) */
      result = (getRegister(rd, reg_current) + imm);
      setFlags(flagAdd, getRegister(rd, reg_current), imm, result, 0);
      putRegister(rd, result, reg_current);
      break;

    case 0X1800: /* SUB (2) */
      result = (getRegister(rd, reg_current) - imm);
      setFlags(flagSub, getRegister(rd, reg_current), imm, result, 1);
      putRegister(rd, result, reg_current);
      break;
  }
}

/*----------------------------------------------------------------------------*/

void data_transfer(uint op_code) {
  uint rd, rm;
  int cf;

  uint address;

  signed int result;

  if ((op_code & 0X1000) == 0) /* NOT load/store */
  {
    if ((op_code & 0X0800) == 0) /* NOT load literal pool */
    {
      if ((op_code & 0X0400) == 0) /* Data processing */
      {
        rd = getRegister((op_code & 7), reg_current);
        rm = getRegister(((op_code >> 3) & 7), reg_current);

        switch (op_code & 0X03C0) /* data processing opcode */
        {
          case 0X0000: /* AND */
            result = rd & rm;
            putRegister(op_code & 7, result, reg_current);
            setNZ(result);
            break;

          case 0X0040: /* EOR */
            result = rd ^ rm;
            putRegister(op_code & 7, result, reg_current);
            setNZ(result);
            break;

          case 0X0080:                    /* LSL (2) */
            cf = ((cpsr & cf_mask) != 0); /* default */
            result = lsl(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cf_mask;
            else
              cpsr = cpsr & ~cf_mask;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X00C0:                    /* LSR (2) */
            cf = ((cpsr & cf_mask) != 0); /* default */
            result = lsr(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cf_mask;
            else
              cpsr = cpsr & ~cf_mask;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X0100:                    /* ASR (2) */
            cf = ((cpsr & cf_mask) != 0); /* default */
            result = asr(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cf_mask;
            else
              cpsr = cpsr & ~cf_mask;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X0140: /* ADC */
            result = rd + rm;
            if ((cpsr & cf_mask) != 0)
              result = result + 1; /* Add CF */
            setFlags(flagAdd, rd, rm, result, cpsr & cf_mask);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X0180: /* SBC */
            result = rd - rm - 1;
            if ((cpsr & cf_mask) != 0)
              result = result + 1;
            setFlags(flagSub, rd, rm, result, cpsr & cf_mask);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X01C0:                    /* ROR */
            cf = ((cpsr & cf_mask) != 0); /* default */
            result = ror(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cf_mask;
            else
              cpsr = cpsr & ~cf_mask;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X0200: /* TST */
            setNZ(rd & rm);
            break;

          case 0X0240: /* NEG */
            result = -rm;
            putRegister(op_code & 7, result, reg_current);
            setFlags(flagSub, 0, rm, result, 1);
            break;

          case 0X0280: /* CMP (2) */
            setFlags(flagSub, rd, rm, rd - rm, 1);
            break;

          case 0X02C0: /* CMN */
            setFlags(flagAdd, rd, rm, rd + rm, 0);
            break;

          case 0X0300: /* ORR */
            result = rd | rm;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X00340: /* MUL */
            result = rm * rd;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X0380: /* BIC */
            result = rd & ~rm;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;

          case 0X03C0: /* MVN */
            result = ~rm;
            setNZ(result);
            putRegister(op_code & 7, result, reg_current);
            break;
        }    /* End of switch */
      } else /* special data processing */
      {      /* NO FLAG UPDATE */
        switch (op_code & 0X0300) {
          case 0X0000: /* ADD (4) high registers */
            rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
            rm = getRegister(((op_code >> 3) & 15), reg_current);
            putRegister(rd, getRegister(rd, reg_current) + rm, reg_current);
            break;

          case 0X0100: /* CMP (3) high registers */
            rd = getRegister((((op_code & 0X0080) >> 4) | (op_code & 7)),
                             reg_current);
            rm = getRegister(((op_code >> 3) & 15), reg_current);
            setFlags(flagSub, rd, rm, rd - rm, 1);
            break;

          case 0X0200: /* MOV (2) high registers */
            rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
            rm = getRegister(((op_code >> 3) & 15), reg_current);

            if (rd == 15)
              rm = rm & 0XFFFFFFFE; /* Tweak mov to PC */
            putRegister(rd, rm, reg_current);
            break;

          case 0X0300: /* BX/BLX Rm */
            bx((op_code >> 3) & 0XF, op_code & 0X0080);
            break;
        } /* End of switch */
      }
    } else /* load from literal pool */
    {      /* LDR PC */
      rd = ((op_code >> 8) & 7);
      address = (((op_code & 0X00FF) << 2)) +
                (getRegister(15, reg_current) & 0XFFFFFFFC);
      putRegister(rd, readMemory(address, 4, false, false, memData),
                  reg_current);
    }
  } else { /* load/store word, halfword, byte, signed byte */
    int rm, rn;
    int data;

    rd = (op_code & 7);
    rn = getRegister(((op_code >> 3) & 7), reg_current);
    rm = getRegister(((op_code >> 6) & 7), reg_current);

    switch (op_code & 0X0E00) {
      case 0X0000: /* STR (2) register */
        writeMemory(rn + rm, getRegister(rd, reg_current), 4, false, memData);
        break;

      case 0X0200: /* STRH (2) register */
        writeMemory(rn + rm, getRegister(rd, reg_current), 2, false, memData);
        break;

      case 0X0400: /* STRB (2) register */
        writeMemory(rn + rm, getRegister(rd, reg_current), 1, false, memData);
        break;

      case 0X0600: /* LDRSB register */
        data = readMemory(rn + rm, 1, true, false, memData); /* Sign ext. */
        putRegister(rd, data, reg_current);
        break;

      case 0X0800: /* LDR (2) register */
        data = readMemory(rn + rm, 4, false, false, memData);
        putRegister(rd, data, reg_current);
        break;

      case 0X0A00: /* LDRH (2) register */
        data = readMemory(rn + rm, 2, false, false, memData); /* Zero ext. */
        putRegister(rd, data, reg_current);
        break;

      case 0X0C00:                                            /* LDRB (2) */
        data = readMemory(rn + rm, 1, false, false, memData); /* Zero ext. */
        putRegister(rd, data, reg_current);
        break;

      case 0X0E00:                                           /* LDRSH (2) */
        data = readMemory(rn + rm, 2, true, false, memData); /* Sign ext. */
        putRegister(rd, data, reg_current);
        break;
    }
  }
}

/*----------------------------------------------------------------------------*/

void transfer0(uint op_code) {
  int rd, rn;
  int location, data;

  rn = getRegister(((op_code >> 3) & 7), reg_current);

  if ((op_code & 0X0800) == 0) /* STR */
  {
    rd = getRegister((op_code & 7), reg_current);
    if ((op_code & 0X1000) == 0) /* STR (1) 5-bit imm */
    {
      location = rn + ((op_code >> 4) & 0X07C); /* shift twice = *4 */
      writeMemory(location, rd, 4, false, memData);
    } else /* STRB (1) */
    {
      location = rn + ((op_code >> 6) & 0X1F);
      writeMemory(location, rd, 1, false, memData);
    }
  } else /* LDR (1) */
  {
    rd = op_code & 7;
    if ((op_code & 0X1000) == 0) {
      location = (rn + ((op_code >> 4) & 0X07C)); /* shift twice = *4 */
      data = readMemory(location, 4, false, false, memData);
    } else /* LDRB (1) */
    {
      location = (rn + ((op_code >> 6) & 0X1F));
      data = readMemory(location, 1, false, false, memData); /* Zero extended */
    }
    putRegister(rd, data, reg_current);
  }
}

/*----------------------------------------------------------------------------*/

void transfer1(uint op_code) {
  int rd, rn;
  int data, location;

  switch (op_code & 0X1800) {
    case 0X0000: /* STRH (1) */
      rn = getRegister((op_code >> 3) & 7, reg_current);
      rd = op_code & 7;
      data = getRegister(rd, reg_current);
      location = rn + ((op_code >> 5) & 0X3E); /* x2 in shift */
      writeMemory(location, data, 2, false, memData);
      break;

    case 0X0800: /* LDRH (1) */
      rd = op_code & 7;
      rn = getRegister((op_code >> 3) & 7, reg_current);
      location = rn + ((op_code >> 5) & 0X3E);               /* x2 in shift */
      data = readMemory(location, 2, false, false, memData); /* Zero extended */
      putRegister(rd, data, reg_current);
      break;

    case 0X1000: /* STR (3) -SP */
      data = getRegister(((op_code >> 8) & 7), reg_current);
      rn = getRegister(13, reg_current); /* SP */
      location = rn + ((op_code & 0X00FF) * 4);
      writeMemory(location, data, 4, false, memData);
      break;

    case 0X1800: /* LDR (4) -SP */
      rd = (op_code >> 8) & 7;
      rn = getRegister(13, reg_current);        /* SP */
      location = rn + ((op_code & 0X00FF) * 4); /* x2 in shift */
      data = readMemory(location, 4, false, false, memData);
      putRegister(rd, data, reg_current);
      break;
  }
}

/*----------------------------------------------------------------------------*/

void sp_pc(uint op_code) {
  int rd, sp, data;

  if ((op_code & 0X1000) == 0) /* ADD SP or PC */
  {
    rd = (op_code >> 8) & 7;

    if ((op_code & 0X0800) == 0) /* ADD(5) -PC */
      data = (getRegister(15, reg_current) & 0XFFFFFFFC) +
             ((op_code & 0X00FF) << 2);
    /* getRegister supplies PC + 2 */
    else /* ADD(6) -SP */
      data = (getRegister(13, reg_current)) + ((op_code & 0X00FF) << 2);
    putRegister(rd, data, reg_current);
  } else /* Adjust SP */
  {
    switch (op_code & 0X0F00) {
      case 0X0000:
        if ((op_code & 0X0080) == 0) /* ADD(7) -SP */
          sp = getRegister(13, reg_current) + ((op_code & 0X7F) << 2);
        else /* SUB(4) -SP */
          sp = getRegister(13, reg_current) - ((op_code & 0X7F) << 2);
        putRegister(13, sp, reg_current);
        break;

      case 0X0400:
      case 0X0500:
      case 0X0C00:
      case 0X0D00: {
        int reg_list;

        reg_list = op_code & 0X000000FF;

        if ((op_code & 0X0800) == 0) /* PUSH */
        {
          if ((op_code & 0X0100) != 0)
            reg_list = reg_list | 0X4000;
          stm(2, 13, reg_list, 1, 0);
        } else /* POP */
        {
          if ((op_code & 0X0100) != 0)
            reg_list = reg_list | 0X8000;
          ldm(1, 13, reg_list, 1, 0);
        }
      } break;

      case 0X0E00: /* Breakpoint */
        //      fprintf(stderr, "Breakpoint\n");
        breakpoint();
        break;

      case 0X0100:
      case 0X0200:
      case 0X0300:
      case 0X0600:
      case 0X0700:
      case 0X0800:
      case 0X0900:
      case 0X0A00:
      case 0X0B00:
      case 0X0F00:
        //      fprintf(stderr, "Undefined\n");
        undefined();
        break;
    }
  }
}

/*----------------------------------------------------------------------------*/

void lsm_b(uint op_code) {
  uint offset;

  if ((op_code & 0X1000) == 0) {
    if ((op_code & 0X0800) == 0) /* STM (IA) */
      stm(1, (op_code >> 8) & 7, op_code & 0X000000FF, 1, 0);
    else /* LDM (IA) */
      ldm(1, (op_code >> 8) & 7, op_code & 0X000000FF, 1, 0);
  } else /* conditional BRANCH B(1) */
  {
    if ((op_code & 0X0F00) != 0X0F00) /* Branch, not a SWI */
    {
      if (checkCC(op_code >> 8) == true) {
        offset = (op_code & 0X00FF) << 1; /* sign extend */
        if ((op_code & 0X0080) != 0)
          offset = offset | 0XFFFFFE00;

        putRegister(15, getRegister(15, reg_current) + offset, reg_current);
        /* getRegister supplies pc + 2 */
        /*    fprintf(stderr, "%08X", address + 4 + offset); */
      }
    } else /* SWI */
    {
      offset = op_code & 0X00FF;
      /* bodge op_code to pass only SWI No. N.B. no copro in Thumb */
      mySystem(offset);
    }
  }
}

/*----------------------------------------------------------------------------*/

void thumb_branch1(uint op_code, int exchange) {
  int offset, lr;

  lr = getRegister(14, reg_current); /* Retrieve first part of offset */
  offset = lr + ((op_code & 0X07FF) << 1);

  lr = getRegister(15, reg_current) - 2 + 1; /* + 1 to indicate Thumb mode */

  if (exchange == true) {
    cpsr = cpsr & ~tf_mask; /* Change to ARM mode */
    offset = offset & 0XFFFFFFFC;
  }

  putRegister(15, offset, reg_current);
  putRegister(14, lr, reg_current);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void thumb_branch(uint op_code) {
  int offset;

  switch (op_code & 0X1800) {
    case 0X0000: /* B -uncond. B(2)  */
      offset = (op_code & 0X07FF) << 1;
      if ((op_code & 0X0400) != 0)
        offset = offset | 0XFFFFF000; /* sign extend */
      putRegister(15, (getRegister(15, reg_current) + offset), reg_current);
      break;

    case 0X0800: /* BLX */
      if ((op_code & 0X0001) == 0)
        thumb_branch1(op_code, true);
      else
        fprintf(stderr, "Undefined\n");
      break;

    case 0X1000: /* BL prefix */
      BL_prefix = op_code & 0X07FF;
      offset = BL_prefix << 12;

      if ((BL_prefix & 0X0400) != 0)
        offset = offset | 0XFF800000; /* Sign ext. */
      offset = getRegister(15, reg_current) + offset;
      putRegister(14, offset, reg_current);
      break;

    case 0X1800: /* BL */
      thumb_branch1(op_code, false);
      break;
  }
}

/*----------------------------------------------------------------------------*/

/*------------------------------ Charlie's functions--------------------------*/

/*----------------------------------------------------------------------------*/

// Jesus wept.   What was wrong with "readMemory" and "writeMemory"?
// If int < 32 bits the whole lot is broken anyway!

uint getmem32(int number) {
  number = number % RAMSIZE;
  return memory[(number << 2)] | memory[(number << 2) + 1] << 8 |
         memory[(number << 2) + 2] << 16 | memory[(number << 2) + 3] << 24;
}

void setmem32(int number, uint reg) {
  number = number & (RAMSIZE - 1);
  memory[(number << 2) + 0] = (reg >> 0) & 0xff;
  memory[(number << 2) + 1] = (reg >> 8) & 0xff;
  memory[(number << 2) + 2] = (reg >> 16) & 0xff;
  memory[(number << 2) + 3] = (reg >> 24) & 0xff;
}

/*----------------------------------------------------------------------------*/
/* terminal support routines                                                  */

void initBuffer(ringBuffer* buffer) {
  buffer->iHead = 0;
  buffer->iTail = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Measure buffer occupancy                                                   */

int countBuffer(ringBuffer* buffer) {
  int i;
  i = buffer->iHead - buffer->iTail;
  if (i < 0)
    i = i + RING_BUF_SIZE;
  return i;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int putBuffer(ringBuffer* buffer, unsigned char c) {
  int status;
  uint temp;

  temp = (buffer->iHead + 1) % RING_BUF_SIZE;

  if (temp != buffer->iTail) {
    buffer->buffer[temp] = c;
    buffer->iHead = temp;
    status = (0 == 0); /* Okay */
  } else
    status = (0 == 1); /* Buffer full */

  return status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int getBuffer(ringBuffer* buffer, unsigned char* c) {
  int status;

  if (buffer->iTail != buffer->iHead) {
    buffer->iTail =
        (buffer->iTail + 1) % RING_BUF_SIZE; /* Preincrement pointer */
    *c = buffer->buffer[buffer->iTail];
    status = (0 == 0); /* Okay */
  } else
    status = (0 == 1); /* Nothing read */

  return status;
}

/*                                end of jimulator.c                          */
/*============================================================================*/
