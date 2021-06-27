/**
 * @file jimulator.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The emulator associated with KoMo2.
 * @version 1.6
 * @date 2021-06-27
 * @todo check long multiplications
 * @todo Flag checking (immediate ?!)
 * @todo Validation
 * @todo interrupt enable behaviour on exceptions (etc.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "definitions.h"
#include "interface.h"

#define uchar unsigned char

#define NO_OF_BREAKPOINTS 32  // Max 32
#define NO_OF_WATCHPOINTS 4   // Max 32
#define RING_BUF_SIZE 64

typedef struct {
  uint iHead;
  uint iTail;
  uchar buffer[RING_BUF_SIZE];
} ringBuffer;

struct pollfd pollfd;

// Local prototypes

void step();
void comm(struct pollfd*);

void emulSetup();
void saveState(uchar new_status);
void initialise(uint start_address, int initial_mode);
void execute(uint op_code);

// ARM execute

void dataOp(uint op_code);
void clz(uint op_code);
void transfer(uint op_code);
void transferSBHW(uint op_code);
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
constexpr const int instructionLength(const int cpsr, const int tfMask);

uint fetch();
void incPC();
void endianSwap(uint start, uint end);
int readMemory(uint address, int size, bool sign, bool T, int source);
void writeMemory(uint address, int data, int size, bool T, int source);

/* THUMB execute */
void data0(uint op_code);
void data1(uint op_code);
void dataTransfer(uint op_code);
void transfer0(uint op_code);
void transfer1(uint op_code);
void spPC(uint op_code);
void lsmB(uint op_code);
void thumbBranch(uint op_code);

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

int getChar(uchar* to_get);
int sendChar(uchar to_send);
int sendNBytes(int value, int N);
int getNBytes(int* val_ptr, int N);
int getCharArray(int char_number, uchar* data_ptr);
int sendCharArray(int char_number, uchar* data_ptr);

void boardreset();

void initBuffer(ringBuffer*);
int countBuffer(ringBuffer*);
bool putBuffer(ringBuffer*, const uchar);
bool getBuffer(ringBuffer*, uchar*);

// Why add "RAMSIZE", and then get it wrong?!?!
// Memory is modulo this to the monitor; excise and use the proper routines
constexpr const uint memSize = 0X100000;       // 4 MB
constexpr const uint RAMSIZE = 0X100000;       // 4 MB
constexpr const uint reserved_mem = 0X002000;  // 32 KB
constexpr const uint userStack = (memSize - reserved_mem) << 2;
constexpr const uint stackStringAddr = 0X00007000;  // ARM address

constexpr const uint maxInstructions = 10000000;

constexpr const uint nfMask = 0X80000000;
constexpr const uint zfMask = 0X40000000;
constexpr const uint cfMask = 0X20000000;
constexpr const uint vfMask = 0X10000000;
constexpr const uint ifMask = 0X00000080;
constexpr const uint ffMask = 0X00000040;
constexpr const uint modeMask = 0X0000001F;
constexpr const uint tfMask = 0X00000020;  // THUMB bit

constexpr const uint bit31 = 0X80000000;
constexpr const uint bit0 = 0X00000001;

constexpr const uint immMask = 0X02000000;      // original word versions
constexpr const uint immHwMask = 0X00400000;    // half word versions
constexpr const uint dataOpMask = 0X01E00000;   // ALU function code
constexpr const uint dataExtMask = 0X01900000;  // To sort out CMP from MRS
constexpr const uint arithExt = 0X01000000;     // Poss. arithmetic extension
constexpr const uint sMask = 0X00100000;
constexpr const uint rnMask = 0X000F0000;
constexpr const uint rdMask = 0X0000F000;
constexpr const uint rsMask = 0X00000F00;
constexpr const uint rmMask = 0X0000000F;
constexpr const uint op2Mask = 0X00000FFF;
constexpr const uint hwMask = 0X00000020;
constexpr const uint signMask = 0X00000040;

constexpr const uint mulMask = 0X0FC000F0;
constexpr const uint longMulMask = 0X0F8000F0;
constexpr const uint mulOp = 0X00000090;
constexpr const uint longMulOp = 0X00800090;
constexpr const uint mulAccBit = 0X00200000;
constexpr const uint mulSignBit = 0X00400000;
constexpr const uint mulLongBit = 0X00800000;

constexpr const uint sbhwMask = 0X0E000FF0;

constexpr const uint swpMask = 0X0FB00FF0;
constexpr const uint swpOp = 0X01000090;

constexpr const uint preMask = 0X01000000;
constexpr const uint upMask = 0X00800000;
constexpr const uint byteMask = 0X00400000;
constexpr const uint writeBackMask = 0X00200000;
constexpr const uint loadMask = 0X00100000;
constexpr const uint byteSign = 0X00000080;
constexpr const uint hwSign = 0X00008000;

constexpr const uint userMask = 0X00400000;

constexpr const uint linkMask = 0X01000000;
constexpr const uint branchField = 0X00FFFFFF;
constexpr const uint branchSign = 0X00800000;

constexpr const uint undefMask = 0X0E000010;
constexpr const uint undefCode = 0X06000010;

constexpr const int memSystem = 0;  // sources for memory read
constexpr const int memInstruction = 1;
constexpr const int memData = 2;

constexpr const int flagAdd = 1;
constexpr const int flagSub = 2;

constexpr const uint userMode = 0x00000010;
constexpr const uint fiqMode = 0x00000011;
constexpr const uint irqMode = 0x00000012;
constexpr const uint supMode = 0x00000013;
constexpr const uint abtMode = 0x00000017;
constexpr const uint undefMode = 0x0000001B;
constexpr const uint systemMode = 0x0000001F;

constexpr const uint regCurrent = 0;  // Value forces accesses to specific bank
constexpr const uint regUser = 1;     // or system
constexpr const uint regSvc = 2;
constexpr const uint regFiq = 3;
constexpr const uint regIrq = 4;
constexpr const uint regAbt = 5;
constexpr const uint regUndef = 6;

constexpr const uint REGSIZE = 65536;

typedef struct {
  int state;
  uchar cond;
  uchar size;
  int addrA;
  int addrB;
  int dataA[2];
  int dataB[2];
} BreakElement;

constexpr const uint WOTLEN_FEATURES = 1;
constexpr const uint WOTLEN_MEM_SEGS = 1;
constexpr const uint WOTLEN = (8 + 3 * WOTLEN_FEATURES + 8 * WOTLEN_MEM_SEGS);

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
    memSize & 0xFF,
    (memSize >> 8) & 0xFF,  // Memory segment
    (memSize >> 16) & 0xFF,
    (memSize >> 24) & 0xFF};  //  length (W)

BreakElement breakpoints[NO_OF_BREAKPOINTS];
BreakElement watchpoints[NO_OF_WATCHPOINTS];

uint emulBPFlag[2];
uint emulWPFlag[2];

uchar memory[RAMSIZE];

uchar status, oldStatus;
int stepsToGo;    // Number of left steps before halting (0 is infinite)
uint stepsReset;  // Number of steps since last reset
char runFlags;
uchar rtf;
bool breakpointEnable;   // Breakpoints will be checked
bool breakpointEnabled;  // Breakpoints will be checked now
bool runThroughBL;       // Treat BL as a single step
bool runThroughSWI;      // Treat SWI as a single step

uint tubeAddress;

int r[16];
int fiqR[7];
int irqR[2];
int supR[2];
int abtR[2];
int underR[2];
uint cpsr;
uint spsr[32];  // Lots of wasted space - safe for any "mode"

bool printOut;
int runUntilPC, runUntilSP, runUntilMode;  // Used to determine when
uchar runUntilStatus;  //  to finish a `stepped' subroutine, SWI, etc.

uint exceptionPara[9];

int nextFileHandle;
FILE*(fileHandle[20]);

int count;

uint lastAddr;

int glob1, glob2;

int pastOpcAddr[32];  // History buffer of fetched op. code addresses
int pastSize;         // Used size of buffer
int pastOpcPtr;       // Pointer into same
int pastCount;        // Count of hits in instruction history

// Thumb stuff
int PC;
int BLPrefix, BLAddress;
int ARMFlag;

struct pollfd* SWIPoll;  // Pointer to allow SWI to scan input - YUK!

ringBuffer terminal0Tx, terminal0Rx;
ringBuffer terminal1Tx, terminal1Rx;
ringBuffer* terminalTable[16][2];

/**
 * @brief Program entry point.
 * @return int Exit code.
 */
int main(int argc, char** argv) {
  {
    for (int i = 0; i < 16; i++) {
      terminalTable[i][0] = NULL;
      terminalTable[i][1] = NULL;
    }

    initBuffer(&terminal0Tx);  // Initialise terminal
    initBuffer(&terminal0Rx);
    terminalTable[0][0] = &terminal0Tx;
    terminalTable[0][1] = &terminal0Rx;
    initBuffer(&terminal1Tx);  // Initialise terminal
    initBuffer(&terminal1Rx);
    terminalTable[1][0] = &terminal1Tx;
    terminalTable[1][1] = &terminal1Rx;
  }

  pollfd.fd = 0;
  pollfd.events = POLLIN;
  SWIPoll = &pollfd;  // Grubby pass to "mySystem"

  emulSetup();

  emulBPFlag[0] = 0;
  if (NO_OF_BREAKPOINTS == 0) {
    emulBPFlag[1] = 0x00000000;  // C work around
  } else {
    emulBPFlag[1] = (1 << NO_OF_WATCHPOINTS) - 1;
  }

  emulWPFlag[0] = 0;
  if (NO_OF_WATCHPOINTS == 0) {
    emulWPFlag[1] = 0x00000000;  // C work around
  } else {
    emulWPFlag[1] = (1 << NO_OF_WATCHPOINTS) - 1;
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

/**
 * @brief
 */
void step() {
  oldStatus = status;
  executeInstruction();

  // Still running - i.e. no breakpoint (etc.) found
  if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING) {
    // don't count the instructions from now
    if (status == CLIENT_STATE_RUNNING_SWI) {
      if ((getRegisterMonitor(15, regCurrent) == runUntilPC) &&
          (getRegisterMonitor(13, regCurrent) == runUntilSP) &&
          ((getRegisterMonitor(16, regCurrent) & 0x3F) == runUntilMode)) {
        status = runUntilStatus;
      }
    }  // This can have changed status - hence no "else" below

    // OR _BL
    if (status != CLIENT_STATE_RUNNING_SWI) {
      // Count steps unless inside routine
      stepsReset++;

      // Stepping
      if (stepsToGo > 0) {
        stepsToGo--;  // If -decremented- to reach zero, stop
        if (stepsToGo == 0) {
          status = CLIENT_STATE_STOPPED;
        }
      }
    }
  }

  if ((status & CLIENT_STATE_CLASS_MASK) != CLIENT_STATE_CLASS_RUNNING) {
    breakpointEnabled = false;  // No longer running - allow "continue"
  }
}

/**
 * @brief
 * @param command
 */
void monitorOptionsMisc(uchar command) {
  uchar tempchar;
  int temp;
  switch (command & 0x3F) {
    case BR_NOP:
      break;
    case BR_PING:
      if (write(1, "OK00", 4) < 0) {
        std::cout << "Some error occurred!" << std::endl;
      }
      break;
    case BR_WOT_R_U:
      sendCharArray(wotrustring[0], &wotrustring[1]);
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
      sendNBytes(stepsToGo, 4);
      sendNBytes(stepsReset, 4);
      break;

    case BR_PAUSE:
    case BR_STOP:
      if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING) {
        oldStatus = status;
        status = CLIENT_STATE_STOPPED;
      }
      break;

    case BR_CONTINUE:
      if (((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_STOPPED) &&
          (status != CLIENT_STATE_BYPROG))  // Only act if already stopped
        if ((oldStatus = CLIENT_STATE_STEPPING) || (stepsToGo != 0))
          status = oldStatus;
      break;

    case BR_BP_GET:
      sendNBytes(emulBPFlag[0], 4);
      sendNBytes(emulBPFlag[1], 4);
      break;

    case BR_BP_SET: {
      int data[2];
      getNBytes(&data[0], 4);
      getNBytes(&data[1], 4);
      /* Note ordering to avoid temporary variable */
      emulBPFlag[1] = (~emulBPFlag[0] & emulBPFlag[1]) |
                      (emulBPFlag[0] & ((emulBPFlag[1] & ~data[0]) | data[1]));
      emulBPFlag[0] = emulBPFlag[0] & (data[0] | ~data[1]);
    } break;

    case BR_BP_READ:
      getChar(&tempchar);
      temp = tempchar;
      sendChar(breakpoints[temp].cond);
      sendChar(breakpoints[temp].size);
      sendNBytes(breakpoints[temp].addrA, 4);
      sendNBytes(breakpoints[temp].addrB, 4);
      sendNBytes(breakpoints[temp].dataA[0], 4);
      sendNBytes(breakpoints[temp].dataA[1], 4);
      sendNBytes(breakpoints[temp].dataB[0], 4);
      sendNBytes(breakpoints[temp].dataB[1], 4);
      break;

    case BR_BP_WRITE:
      getChar(&tempchar);
      temp = tempchar;
      getChar(&breakpoints[temp].cond);
      getChar(&breakpoints[temp].size);
      getNBytes(&breakpoints[temp].addrA, 4);
      getNBytes(&breakpoints[temp].addrB, 4);
      getNBytes(&breakpoints[temp].dataA[0], 4);
      getNBytes(&breakpoints[temp].dataA[1], 4);
      getNBytes(&breakpoints[temp].dataB[0], 4);
      getNBytes(&breakpoints[temp].dataB[1], 4);
      /* add breakpoint */
      temp = (1 << temp) & ~emulBPFlag[0];
      emulBPFlag[0] |= temp;
      emulBPFlag[1] |= temp;
      break;

    case BR_WP_GET:
      sendNBytes(emulWPFlag[0], 4);
      sendNBytes(emulWPFlag[1], 4);
      break;

    case BR_WP_SET: {
      int data[2];
      getNBytes(&data[0], 4);
      getNBytes(&data[1], 4);
      temp = data[1] & ~data[0];
      emulWPFlag[0] &= ~temp;
      emulWPFlag[1] |= temp;
      temp = data[0] & emulWPFlag[0];
      emulWPFlag[1] = (emulWPFlag[1] & ~temp) | (data[1] & temp);
    } break;

    case BR_WP_READ:
      getChar(&tempchar);
      temp = tempchar;
      sendChar(watchpoints[temp].cond);
      sendChar(watchpoints[temp].size);
      sendNBytes(watchpoints[temp].addrA, 4);
      sendNBytes(watchpoints[temp].addrB, 4);
      sendNBytes(watchpoints[temp].dataA[0], 4);
      sendNBytes(watchpoints[temp].dataA[1], 4);
      sendNBytes(watchpoints[temp].dataB[0], 4);
      sendNBytes(watchpoints[temp].dataB[1], 4);
      break;

    case BR_WP_WRITE:
      getChar(&tempchar);
      temp = tempchar;
      getChar(&watchpoints[temp].cond);
      getChar(&watchpoints[temp].size);
      getNBytes(&watchpoints[temp].addrA, 4);
      getNBytes(&watchpoints[temp].addrB, 4);
      getNBytes(&watchpoints[temp].dataA[0], 4);
      getNBytes(&watchpoints[temp].dataA[1], 4);
      getNBytes(&watchpoints[temp].dataB[0], 4);
      getNBytes(&watchpoints[temp].dataB[1], 4);
      temp = 1 << temp & ~emulWPFlag[0];
      emulWPFlag[0] |= temp;
      emulWPFlag[1] |= temp;
      break;

    case BR_FR_WRITE: {
      uchar device, length;
      ringBuffer* pBuff;

      getChar(&device);
      pBuff = terminalTable[device][1];
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
      uchar device, max_length;
      uint i, length, available;
      ringBuffer* pBuff;

      getChar(&device);
      pBuff = terminalTable[device][0];
      getChar(&max_length);
      available = countBuffer(&terminal0Tx); /* See how many chars we have */
      if (pBuff == NULL)
        length = 0; /* Kill if no corresponding buffer */
      else
        length = MIN(available, max_length); /* else clip to message max. */
      sendChar(length);
      for (i = 0; i < length; i++) /* Send zero or more characters */
      {
        uchar c;
        getBuffer(pBuff, &c);
        sendChar(c);
      }
    } break;

    default:
      break;
  }
}

/**
 * @brief
 * @param c
 */
void monitorMemory(uchar c) {
  int addr;
  uchar* pointer;
  int size;

  getNBytes(&addr, 4);  // Start address really
  if ((c & 0x30) == 0x10) {
    int temp;
    int reg_bank, reg_number;

    switch (addr & 0xE0) {
      case 0x00:
        reg_bank = regCurrent;
        break;
      case 0x20:
        reg_bank = regUser;
        break;
      case 0x40:
        reg_bank = regSvc;
        break;
      case 0x60:
        reg_bank = regAbt;
        break;
      case 0x80:
        reg_bank = regUndef;
        break;
      case 0xA0:
        reg_bank = regIrq;
        break;
      case 0xC0:
        reg_bank = regFiq;
        break;
      default:
        reg_bank = regCurrent;
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

/**
 * @brief
 * @param c
 */
void monitorBreakpoints(uchar c) {
  runFlags = c & 0x3F;
  breakpointEnable = (runFlags & 0x10) != 0;
  breakpointEnabled = (runFlags & 0x01) != 0; /* Break straight away */
  runThroughBL = (runFlags & 0x02) != 0;
  runThroughSWI = (runFlags & 0x04) != 0;
  getNBytes(&stepsToGo, 4);
  if (stepsToGo == 0)
    status = CLIENT_STATE_RUNNING;
  else
    status = CLIENT_STATE_STEPPING;
}

/**
 * @brief
 * @param pPollfd
 */
void comm(struct pollfd* pPollfd) {
  uchar c;

  if (poll(pPollfd, 1, 0) > 0) {
    if (read(0, &c, 1) < 0) {
      std::cout << "Some error occurred!" << std::endl;
    }  // Look at error return - find EOF & exit
    switch (c & 0xC0) {
      case 0x00:
        monitorOptionsMisc(c);
        break;
      case 0x40:
        monitorMemory(c);
        break;
      case 0x80:
        monitorBreakpoints(c);
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
int getChar(uchar* to_get) {
  return getCharArray(1, to_get);
}

/**
 * @brief Send 1 character to host
 * @param to_send
 * @return int
 */
int sendChar(uchar to_send) {
  return sendCharArray(1, &to_send);
}

/**
 * @brief Sends N bytes from the supplied value to the host (??), LSB first.
 * @param value
 * @param N
 * @return int The number of bytes believed received successfully (i.e. N=>"Ok")
 */
int sendNBytes(int value, int N) {
  uchar buffer[MAX_SERIAL_WORD];
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
  uchar buffer[MAX_SERIAL_WORD];
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
int getCharArray(int char_number, uchar* data_ptr) {
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
int sendCharArray(int char_number, uchar* data_ptr) {
  if (write(1, data_ptr, char_number) < 0) {
    std::cout << "Some error occurred!" << std::endl;
  }

  return char_number;  // send char array to the board
}

/**
 * @brief
 */
void emulSetup() {
  glob1 = 0;
  glob2 = 0;

  for (int i = 0; i < 32; i++) {
    pastOpcAddr[i] = 1;  // Illegal op. code address
  }
  pastOpcPtr = 0;
  pastCount = 0;
  pastSize = 4;

  int initialMode = 0xC0 | supMode;
  printOut = false;

  nextFileHandle = 1;

  initialise(0, initialMode);
}

/**
 * @brief
 *
 * @param instr_addr
 * @param instr
 * @return true
 * @return false
 */
bool checkBreakpoint(uint instr_addr, uint instr) {
  bool mayBreak = false;

  for (int i = 0; (i < NO_OF_BREAKPOINTS) && !mayBreak; i++) {
    mayBreak = ((emulBPFlag[0] & emulBPFlag[1] & (1 << i)) !=
                0);  // Breakpoint is active

    // Try address comparison
    if (mayBreak) {
      switch (breakpoints[i].cond & 0x0C) {
        case 0x00:
        case 0x04:
          mayBreak = false;
          break;
        // Case of between address A and address B
        case 0x08:
          if ((instr_addr < breakpoints[i].addrA) ||
              (instr_addr > breakpoints[i].addrB)) {
            mayBreak = false;
          }
          break;
        // case of mask
        case 0x0C:
          if ((instr_addr & breakpoints[i].addrB) != breakpoints[i].addrA) {
            mayBreak = false;
          }
          break;
      }
    }

    // Try data comparison
    if (mayBreak) {
      switch (breakpoints[i].cond & 0x03) {
        case 0x00:
          mayBreak = false;
          break;

        case 0x01:
          mayBreak = false;
          break;

        case 0x02:  // Case of between data A and data B
          if ((instr < breakpoints[i].dataA[0]) ||
              (instr > breakpoints[i].dataB[0])) {
            mayBreak = false;
          }
          break;

        case 0x03:  // Case of mask
          if ((instr & breakpoints[i].dataB[0]) != breakpoints[i].dataA[0]) {
            mayBreak = false;
          }
          break;
      }
    }
  }
  return mayBreak;
}

/**
 * @brief
 */
void executeInstruction() {
  int i;

  uint instr_addr =
      getRegister(15, regCurrent) - instructionLength(cpsr, tfMask);
  lastAddr = getRegister(15, regCurrent) - instructionLength(cpsr, tfMask);

  /* FETCH */
  auto instr = fetch();

  if ((breakpointEnabled) && (status != CLIENT_STATE_RUNNING_SWI)) {
    if (checkBreakpoint(instr_addr, instr)) {
      status = CLIENT_STATE_BREAKPOINT;
      return;
    }
  }
  breakpointEnabled = breakpointEnable; /* More likely after first fetch */

  /* BL instruction */
  if (((instr & 0x0F000000) == 0x0B000000) && runThroughBL) {
    saveState(CLIENT_STATE_RUNNING_BL);
  } else {
    if (((instr & 0x0F000000) == 0x0F000000) && runThroughSWI) {
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
  runUntilPC = getRegister(15, regCurrent);  // Incremented once: correct here
  runUntilSP = getRegister(13, regCurrent);
  runUntilMode = getRegister(16, regCurrent) & 0x3F;  // Just the mode bits
  runUntilStatus = status;
  status = new_status;
}

/**
 * @brief
 */
void boardreset() {
  stepsReset = 0;
  initialise(0, supMode);
}

/**
 * @brief
 * @param start_address
 * @param initial_mode
 */
void initialise(uint start_address, int initial_mode) {
  cpsr = 0X000000C0 | initial_mode;  // Disable interrupts
  r[15] = start_address;
  oldStatus = CLIENT_STATE_RESET;
  status = CLIENT_STATE_RESET;
}

/**
 * @brief
 * @param op_code
 */
void execute(uint op_code) {
  incPC(); /* Easier here than later */

  /* ARM or THUMB ? */
  if ((cpsr & tfMask) != 0) /* Thumb */
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
        dataTransfer(op_code);
        break;
      case 0X6000:
        transfer0(op_code);
        break;
      case 0X8000:
        transfer1(op_code);
        break;
      case 0XA000:
        spPC(op_code);
        break;
      case 0XC000:
        lsmB(op_code);
        break;
      case 0XE000:
        thumbBranch(op_code);
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

/**
 * @brief
 * @param op_code
 * @return int
 */
int isItSBHW(uint op_code) {
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

/**
 * @brief
 * @param op_code
 */
void dataOp(uint op_code) {
  int operation;

  if (((op_code & mulMask) == mulOp) ||
      ((op_code & longMulMask) == longMulOp)) {
    myMulti(op_code);
  } else if (isItSBHW(op_code) == true) {
    transferSBHW(op_code);
  } else if ((op_code & swpMask) == swpOp) {
    swap(op_code);
  } else {
    operation = (op_code & dataOpMask) >> 21;

    /* TST, TEQ, CMP, CMN - all lie in following range, but have S set */
    if ((op_code & dataExtMask) == arithExt) /* PSR transfers OR BX */
    {
      if ((op_code & 0X0FBF0FFF) == 0X010F0000) {
        mrs(op_code); /* MRS */
      } else if (((op_code & 0X0DB6F000) == 0X0120F000) &&
                 ((op_code & 0X02000010) != 0X00000010)) {
        msr(op_code);                                  /* MSR */
      } else if ((op_code & 0X0FFFFFD0) == 0X012FFF10) /* BX/BLX */
      {
        bx(op_code & rmMask, op_code & 0X00000020);
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

/**
 * @brief
 * @param op_code
 */
void transferSBHW(uint op_code) {
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

  rd = ((op_code & rdMask) >> 12);

  address = getRegister(((op_code & rnMask) >> 16), regCurrent);

  offset = transferOffset(op_code & op2Mask, op_code & upMask,
                          op_code & immHwMask, true);

  if ((op_code & preMask) != 0)
    address = address + offset; /* pre-index */

  if ((op_code & loadMask) == 0) /* store */
    writeMemory(address, getRegister(rd, regCurrent), size, false, memData);
  else /* load */
    putRegister(rd, readMemory(address, size, sign, false, memData),
                regCurrent);
  /* post index */

  if ((op_code & preMask) == 0) /* post index with writeback */
    putRegister((op_code & rnMask) >> 16, address + offset, regCurrent);
  else if ((op_code & writeBackMask) != 0)
    putRegister((op_code & rnMask) >> 16, address, regCurrent);
}

/**
 * @brief
 * @param op_code
 */
void mrs(uint op_code) {
  if ((op_code & 0X00400000) == 0) {
    putRegister((op_code & rdMask) >> 12, cpsr, regCurrent);
  } else {
    putRegister((op_code & rdMask) >> 12, spsr[cpsr & modeMask], regCurrent);
  }
}

/**
 * @brief
 * @param op_code
 */
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
  if ((cpsr & modeMask) == 0X10)
    mask = mask & 0XF0000000; /* User mode */

  if ((op_code & immMask) == 0) /* Test applies for both cases */
    source = getRegister(op_code & rmMask, regCurrent) & mask;
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
    spsr[cpsr & modeMask] = (spsr[cpsr & modeMask] & ~mask) | source;
}

/**
 * @brief
 * @param Rm
 * @param link Link is performed if "link" is NON-ZERO
 */
void bx(uint Rm, int link) {
  int offset, t_bit;

  int PC = getRegister(15, regCurrent);

  if ((cpsr & tfMask) != 0) {
    PC = PC - 2;
    PC = PC | 1;
  } /* Remember Thumb mode */
  else {
    PC = PC - 4;
  }

  offset = getRegister(Rm, regCurrent) & 0XFFFFFFFE;
  t_bit = getRegister(Rm, regCurrent) & 0X00000001;

  if (t_bit == 1)
    cpsr = cpsr | tfMask;
  else
    cpsr = cpsr & ~tfMask;

  putRegister(15, offset, regCurrent); /* Update PC */

  if (link != 0) {
    putRegister(14, PC, regCurrent); /* Link if BLX */
  }
}

/**
 * @brief
 * @param op_code
 */
void myMulti(uint op_code) {
  int acc;

  if ((op_code & mulLongBit) == 0) /* Normal */
  {
    acc = getRegister(op_code & rmMask, regCurrent) *
          getRegister((op_code & rsMask) >> 8, regCurrent);

    if ((op_code & mulAccBit) != 0)
      acc = acc + getRegister((op_code & rdMask) >> 12, regCurrent);

    putRegister((op_code & rnMask) >> 16, acc, regCurrent);

    if ((op_code & sMask) != 0)
      setNZ(acc); /* Flags */
  } else          /* Long */
  {
    uint Rm, Rs, th, tm, tl;
    int sign;

    Rm = getRegister(op_code & rmMask, regCurrent);
    Rs = getRegister((op_code & rsMask) >> 8, regCurrent);

    sign = 0;
    if ((op_code & mulSignBit) != 0) /* Signed */
    {
      if ((Rm & bit31) != 0) {
        Rm = ~Rm + 1;
        sign = 1;
      }
      if ((Rs & bit31) != 0) {
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

    if ((op_code & mulAccBit) != 0) {
      tm = tl + getRegister((op_code & rdMask) >> 12, regCurrent);
      if (tm < tl)
        th = th + 1; /* Propagate carry */
      tl = tm;
      th = th + getRegister((op_code & rnMask) >> 16, regCurrent);
    }

    putRegister((op_code & rdMask) >> 12, tl, regCurrent);
    putRegister((op_code & rnMask) >> 16, th, regCurrent);

    if ((op_code & sMask) != 0)
      setNZ(th | (((tl >> 16) | tl) & 0X0000FFFF)); /* Flags */
  }
}

/**
 * @brief
 * @param op_code
 */
void swap(uint op_code) {
  uint address, data, size;

  address = getRegister((op_code & rnMask) >> 16, regCurrent);

  if ((op_code & byteMask) != 0)
    size = 1;
  else
    size = 4;

  data = readMemory(address, size, false, false, memData);
  writeMemory(address, getRegister(op_code & rmMask, regCurrent), size, false,
              memData);
  putRegister((op_code & rdMask) >> 12, data, regCurrent);
}

/**
 * @brief
 * @param op_code
 * @param operation
 */
void normalDataOp(uint op_code, int operation) {
  int rd, a, b, mode;
  int shift_carry;
  int CPSR_special;

  mode = cpsr & modeMask;
  CPSR_special = false;
  shift_carry = 0;
  a = getRegister((op_code & rnMask) >> 16,
                  regCurrent);  // force_user = false

  if ((op_code & immMask) == 0) {
    b = bReg(op_code & op2Mask, &shift_carry);
  } else {
    b = bImmediate(op_code & op2Mask, &shift_carry);
  }

  // R15s
  switch (operation) {
    case 0X0:
      rd = a & b;
      break;  // AND
    case 0X1:
      rd = a ^ b;
      break;  // EOR
    case 0X2:
      rd = a - b;
      break;  // SUB
    case 0X3:
      rd = b - a;
      break;  // RSB
    case 0X4:
      rd = a + b;
      break;  // ADD
    case 0X5:
      rd = a + b;
      if ((cpsr & cfMask) != 0)
        rd = rd + 1;
      break;  // ADC
    case 0X6:
      rd = a - b - 1;
      if ((cpsr & cfMask) != 0)
        rd = rd + 1;
      break;  // SBC
    case 0X7:
      rd = b - a - 1;
      if ((cpsr & cfMask) != 0)
        rd = rd + 1;
      break;  // RSC
    case 0X8:
      rd = a & b;
      break;  // TST
    case 0X9:
      rd = a ^ b;  // TEQ

      // TEQP
      if ((op_code & rdMask) == 0XF000) {
        CPSR_special = true;
        if (mode != userMode)
          cpsr = spsr[mode];
      }
      break;
    case 0XA:
      rd = a - b;
      break;  // CMP
    case 0XB:
      rd = a + b;
      break;  // CMN
    case 0XC:
      rd = a | b;
      break;  // ORR
    case 0XD:
      rd = b;
      break;  // MOV
    case 0XE:
      rd = a & ~b;
      break;  // BIC
    case 0XF:
      rd = ~b;
      break;  // MVN
  }

  // Return result unless a compare
  if ((operation & 0XC) != 0X8) {
    putRegister((op_code & rdMask) >> 12, rd, regCurrent);
  }

  // S-bit && Want to change CPSR
  if (((op_code & sMask) != 0) && (CPSR_special != true)) {
    // PC and S-bit
    if (((op_code & rdMask) >> 12) == 0XF) {
      // restore saved CPSR
      if (mode != userMode) {
        cpsr = spsr[mode];
      } else {
        fprintf(stderr, "SPSR_user read attempted\n");
      }
    }
    // other dest. registers
    else {
      switch (operation) {  // LOGICALs
        case 0X0:           // AND
        case 0X1:           // EOR
        case 0X8:           // TST
        case 0X9:           // TEQ
        case 0XC:           // ORR
        case 0XD:           // MOV
        case 0XE:           // BIC
        case 0XF:           // MVN
          setNZ(rd);
          if (shift_carry == true) {
            cpsr = cpsr | cfMask;  // CF := output
          } else {
            cpsr = cpsr & ~cfMask;  // from shifter
          }
          break;

        case 0X2:  // SUB
        case 0XA:  // CMP
          setFlags(flagSub, a, b, rd, 1);
          break;

        case 0X6:  // SBC - Needs more testing
          setFlags(flagSub, a, b, rd, cpsr & cfMask);
          break;

        case 0X3:  // RSB
          setFlags(flagSub, b, a, rd, 1);
          break;

        case 0X7:  // RSC
          setFlags(flagSub, b, a, rd, cpsr & cfMask);
          break;

        case 0X4:  // ADD
        case 0XB:  // CMN
          setFlags(flagAdd, a, b, rd, 0);
          break;

        case 0X5:  // ADC
          setFlags(flagAdd, a, b, rd, cpsr & cfMask);
          break;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/
/* shift type: 00 = LSL, 01 = LSR, 10 = ASR, 11 = ROR                         */

int bReg(int op2, int* cf) {
  uint shift_type, reg, distance, result;
  reg = getRegister(op2 & 0X00F, regCurrent); /* Register */
  shift_type = (op2 & 0X060) >> 5;            /* Type of shift */
  if ((op2 & 0X010) == 0) {                   /* Immediate value */
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
    distance = (getRegister((op2 & 0XF00) >> 8, regCurrent) & 0XFF);
  /* Register value */

  *cf = ((cpsr & cfMask) != 0); /* Previous carry */
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
      if ((cpsr & cfMask) == 0)
        result = result & ~bit31;
      else
        result = result | bit31;
      *cf = ((reg & bit0) != 0);
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
    *cf = ((cpsr & cfMask) != 0); /* Previous carry */
  else
    *cf = (((x >> (y - 1)) & bit0) != 0);
  if (*cf)
    *cf = true;
  else
    *cf = false;            /* Change to "bool" */
  return ror(x, y, &dummy); /* Circular rotation */
}

/*----------------------------------------------------------------------------*/

void clz(uint op_code) {
  int i, j;

  j = getRegister(op_code & rmMask, regCurrent);

  if (j == 0)
    i = 32;
  else {
    i = 0;
    while ((j & 0X80000000) == 0) {
      i++;
      j = j << 1;
    }
  }

  putRegister((op_code & rdMask) >> 12, i, regCurrent);
}

/*----------------------------------------------------------------------------*/

void transfer(uint op_code) {
  uint address;
  int offset, rd, size;
  bool T;

  if ((op_code & undefMask) == undefCode) {
    undefined();
  } else {
    if ((op_code & byteMask) == 0) {
      size = 4;
    } else {
      size = 1;
    }

    T = (((op_code & preMask) == 0) && ((op_code & writeBackMask) != 0));
    rd = (op_code & rdMask) >> 12;
    address = getRegister((op_code & rnMask) >> 16, regCurrent);
    offset = transferOffset(op_code & op2Mask, op_code & upMask,
                            op_code & immMask, false);

    if ((op_code & preMask) != 0) {
      address = address + offset;  // Pre-index
    }

    if ((op_code & loadMask) == 0) {
      writeMemory(address, getRegister(rd, regCurrent), size, T, memData);
    } else {
      putRegister(rd, readMemory(address, size, false, T, memData), regCurrent);
    }

    // Post-index
    if ((op_code & preMask) == 0) {
      putRegister((op_code & rnMask) >> 16, address + offset, regCurrent);
    } else if ((op_code & writeBackMask) != 0) {
      putRegister((op_code & rnMask) >> 16, address, regCurrent);
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
  if ((op_code & loadMask) == 0) {
    stm((op_code & 0X01800000) >> 23, (op_code & rnMask) >> 16,
        op_code & 0X0000FFFF, op_code & writeBackMask, op_code & userMask);
  } else {
    ldm((op_code & 0X01800000) >> 23, (op_code & rnMask) >> 16,
        op_code & 0X0000FFFF, op_code & writeBackMask, op_code & userMask);
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
    if ((source & bit0) != 0) {
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

  address = getRegister(Rn, regCurrent);
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
    putRegister(Rn, new_base, regCurrent);
  }

  // Force user unless R15 in list
  if (hat && !r15_inc) {
    force_user = regUser;
  } else {
    force_user = regCurrent;
  }

  reg = 0;

  while (reg_list != 0) {
    if ((reg_list & bit0) != 0) {
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
      cpsr = cpsr | tfMask;  // data left over from last load
    } else {
      cpsr = cpsr & ~tfMask;  // used to set instruction set
    }

    if (hat) {
      cpsr = spsr[cpsr & modeMask];  // and if S bit set
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

  address = getRegister(Rn, regCurrent);
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
      putRegister(Rn, new_base, regCurrent);
    }
  }

  if (hat != 0) {
    force_user = regUser;
  } else {
    force_user = regCurrent;
  }

  reg = 0;

  while (reg_list != 0) {
    if ((reg_list & bit0) != 0) {
      writeMemory(address, getRegister(reg, force_user), 4, false, memData);
      address = address + 4;
    }

    reg_list = reg_list >> 1;
    reg = reg + 1;
  }

  if (special)
    putRegister(Rn, new_base, regCurrent);
}

/**
 * @brief
 * @param opCode
 */
void branch(uint opCode) {
  int PC = getRegister(15, regCurrent);  // Get this now in case mode changes

  if (((opCode & linkMask) != 0) || ((opCode & 0XF0000000) == 0XF0000000)) {
    putRegister(14, getRegister(15, regCurrent) - 4, regCurrent);
  }

  int offset = (opCode & branchField) << 2;

  if ((opCode & branchSign) != 0) {
    offset = offset | (~(branchField << 2) & 0XFFFFFFFC);  // sign extend
  }

  // Other BLX fix-up
  if ((opCode & 0XF0000000) == 0XF0000000) {
    offset = offset | ((opCode >> 23) & 2);
    cpsr = cpsr | tfMask;
  }

  putRegister(15, PC + offset, regCurrent);
}

/**
 * @brief
 * @param c
 * @return true
 * @return false
 */
bool swiCharacterPrint(char c) {
  while (!putBuffer(&terminal0Tx, c)) {
    if (status == CLIENT_STATE_RESET) {
      return false;
    } else {
      comm(SWIPoll);  // If stalled, retain monitor communications
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

/**
 * @brief
 * @param op_code
 */
void mySystem(uint op_code) {
  int temp;

  if (((op_code & 0X0F000000) == 0X0E000000)
      /* bodge to allow Thumb to use this code */
      || ((op_code & 0X0F000000) == 0X0C000000) ||
      ((op_code & 0X0F000000) == 0X0D000000)) { /* Coprocessor op.s */
    fprintf(stderr, "whoops -undefined \n");
    undefined();
  } else {
    if (printOut) {
      fprintf(stderr, "\n*** SWI CALL %06X ***\n\n", op_code & 0X00FFFFFF);
    }

    switch (op_code & 0X00FFFFFF) {
      // Output character R0 (to terminal)
      case 0:
        putRegister(15, getRegister(15, regCurrent) - 8, regCurrent);
        swiCharacterPrint(getRegister(0, regCurrent) & 0XFF);

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, regCurrent),
                      regCurrent);  // Correct PC
        }
        break;

      // Input character R0 (from terminal)
      case 1: {
        uchar c;
        putRegister(15, getRegister(15, regCurrent) - 8, regCurrent);
        // Bodge PC so that stall looks `correct'
        while ((!getBuffer(&terminal0Rx, &c)) &&
               (status != CLIENT_STATE_RESET)) {
          comm(SWIPoll);
        }

        if (status != CLIENT_STATE_RESET) {
          putRegister(0, c & 0XFF, regCurrent);
          putRegister(15, getRegister(15, regCurrent),
                      regCurrent);  // Correct PC
        }
      } break;

      // Halt
      case 2:
        status = CLIENT_STATE_BYPROG;
        break;

      // Print string @R0 (to terminal)
      case 3: {
        putRegister(15, getRegister(15, regCurrent) - 8, regCurrent);
        uint str_ptr = getRegister(0, regCurrent);

        char c;
        while (
            ((c = readMemory(str_ptr, 1, false, false, memSystem)) != '\0') &&
            (status != CLIENT_STATE_RESET)) {
          swiCharacterPrint(c);  // Returns if reset
          str_ptr++;
        }

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, regCurrent),
                      regCurrent);  // Correct PC
        }
      } break;

      // Decimal print R0
      case 4: {
        putRegister(15, getRegister(15, regCurrent) - 8, regCurrent);
        const uint number = getRegister(0, regCurrent);
        number == 0 ? swiCharacterPrint('0') : swiDecimalPrint(number);

        if (status != CLIENT_STATE_RESET) {
          putRegister(15, getRegister(15, regCurrent),
                      regCurrent);  // Correct PC
        }
      } break;

      default:
        if (printOut) {
          fprintf(stderr, "Un-trapped SWI call %06X\n", op_code & 0X00FFFFFF);
        }

        spsr[supMode] = cpsr;
        cpsr = (cpsr & ~modeMask) | supMode;
        cpsr = cpsr & ~tfMask;  // Always in ARM mode
        putRegister(14, getRegister(15, regCurrent) - 4, regCurrent);
        putRegister(15, 8, regCurrent);
        break;
    }
  }
}

/**
 * @brief This is the breakpoint instruction.
 */
void breakpoint() {
  spsr[abtMode] = cpsr;
  cpsr = (cpsr & ~modeMask & ~tfMask) | abtMode;
  putRegister(14, getRegister(15, regCurrent) - 4, regCurrent);
  putRegister(15, 12, regCurrent);
}

/**
 * @brief
 */
void undefined() {
  spsr[undefMode] = cpsr;
  cpsr = (cpsr & ~modeMask & ~tfMask) | undefMode;
  putRegister(14, getRegister(15, regCurrent) - 4, regCurrent);
  putRegister(15, 4, regCurrent);
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
    cpsr = cpsr | zfMask;
  } else {
    cpsr = cpsr & ~zfMask;
  }

  if ((value & bit31) != 0) {
    cpsr = cpsr | nfMask;
  } else {
    cpsr = cpsr & ~nfMask;
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
    cpsr = cpsr & ~cfMask;
  else
    cpsr = cpsr | cfMask;
}

/**
 * @brief
 * @param a
 * @param b
 * @param rd
 */
void setVF_ADD(int a, int b, int rd) {
  cpsr = cpsr & ~vfMask;  // Clear VF
  if (((~(a ^ b) & (a ^ rd)) & bit31) != 0) {
    cpsr = cpsr | vfMask;
  }
}

/**
 * @brief
 * @param a
 * @param b
 * @param rd
 */
void setVF_SUB(int a, int b, int rd) {
  cpsr = cpsr & ~vfMask;  // Clear VF
  if ((((a ^ b) & (a ^ rd)) & bit31) != 0) {
    cpsr = cpsr | vfMask;
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

/**
 * @brief
 * @param cpsr
 * @return true
 * @return false
 */
constexpr const bool zf(const int cpsr) {
  if ((zfMask & cpsr) != 0) {
    return true;
  }

  return false;
}

/**
 * @brief
 * @param cpsr
 * @return true
 * @return false
 */
constexpr const bool cf(const int cpsr) {
  if ((cfMask & cpsr) != 0) {
    return true;
  }

  return false;
}

/**
 * @brief
 * @param cpsr
 * @return true
 * @return false
 */
constexpr const bool nf(const int cpsr) {
  if ((nfMask & cpsr) != 0) {
    return true;
  }

  return false;
}

/**
 * @brief
 * @param cpsr
 * @return true
 * @return false
 */
constexpr const bool vf(const int cpsr) {
  if ((vfMask & cpsr) != 0) {
    return true;
  }

  return false;
}

/**
 * @brief Get the Register object
 * @param reg_no
 * @param force_mode
 * @return int
 */
int getRegister(int reg_no, int force_mode) {
  int mode, value;

  switch (force_mode) {
    case regCurrent:
      mode = cpsr & modeMask;
      break;
    case regUser:
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
    case regUndef:
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
          value = fiqR[reg_no - 8];
        }
        break;

      case irqMode:
        if (reg_no < 13) {
          value = r[reg_no];
        } else {
          value = irqR[reg_no - 13];
        }
        break;

      case supMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = supR[reg_no - 13];
        }
        break;

      case abtMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = abtR[reg_no - 13];
        }
        break;

      case undefMode:
        if (reg_no < 13) {
          value = r[reg_no];

        } else {
          value = underR[reg_no - 13];
        }
        break;
    }
  } else {
    value = r[15] + instructionLength(cpsr, tfMask);
  }

  return value;
}

/**
 * @brief Modified "getRegister" to give unadulterated copy of PC
 * @param reg_no
 * @param force_mode
 * @return int
 */
int getRegisterMonitor(int reg_no, int force_mode) {
  if (reg_no != 15) {
    return getRegister(reg_no, force_mode);
  } else {
    return r[15];  // PC access
  }
}

/**
 * @brief Write to a specified processor register
 * @param reg_no
 * @param value
 * @param force_mode
 */
void putRegister(int reg_no, int value, int force_mode) {
  int mode;

  switch (force_mode) {
    case regCurrent:
      mode = cpsr & modeMask;
      break;
    case regUser:
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
    case regUndef:
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
          fiqR[reg_no - 8] = value;
        break;

      case irqMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          irqR[reg_no - 13] = value;
        break;

      case supMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          supR[reg_no - 13] = value;
        break;

      case abtMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          abtR[reg_no - 13] = value;
        break;

      case undefMode:
        if (reg_no < 13)
          r[reg_no] = value;
        else
          underR[reg_no - 13] = value;
        break;
    }
  } else
    r[15] = value & 0XFFFFFFFE; /* Lose bottom bit, but NOT mode specific! */
}

/**
 * @brief Return the length, in bytes, of the currently expected instruction.
 * @return int 4 for ARM, 2 for Thumb.
 */
constexpr const int instructionLength(const int cpsr, const int tfMask) {
  if ((cpsr & tfMask) == 0) {
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
      (getRegister(15, regCurrent) - instructionLength(cpsr, tfMask)),
      instructionLength(cpsr, tfMask), false, false, memInstruction);

  for (int i = 0; i < 32; i++) {
    if (pastOpcAddr[i] ==
        getRegister(15, regCurrent) - instructionLength(cpsr, tfMask)) {
      pastCount++;
      i = 32;  // bodged escape from loop
    }
  }

  pastOpcAddr[pastOpcPtr++] =
      getRegister(15, regCurrent) - instructionLength(cpsr, tfMask);
  pastOpcPtr = pastOpcPtr % pastSize;

  return op_code;
}

/**
 * @brief getRegister returns PC+4 for ARM & PC+2 for THUMB.
 */
void incPC() {
  putRegister(15, getRegister(15, regCurrent), regCurrent);
}

/**
 * @brief
 * @param start
 * @param end
 */
void endianSwap(const uint start, const uint end) {
  for (uint i = start; i < end; i++) {
    uint j = getmem32(i);
    setmem32(i, ((j >> 24) & 0X000000FF) | ((j >> 8) & 0X0000FF00) |
                    ((j << 8) & 0X00FF0000) | ((j << 24) & 0XFF000000));
  }
}

/**
 * @brief
 * @param address
 * @param size
 * @param sign
 * @param T
 * @param source indicates type of read {memSystem, memInstruction, memData}
 * @return int
 */
int readMemory(uint address, int size, bool sign, bool T, int source) {
  int data, alignment;

  if (address < memSize) {
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
    if ((runFlags & 0x20) && (source == memData)) {
      if (checkWatchpoints(address, data, size, 1)) {
        status = CLIENT_STATE_WATCHPOINT;
      }
    }
  } else {
    data = 0X12345678;
    printOut = false;
  }

  return data;
}

/**
 * @brief
 * @param address
 * @param data
 * @param size
 * @param T
 * @param source
 */
void writeMemory(uint address, int data, int size, bool T, int source) {
  uint mask;

  if ((address == tubeAddress) &&
      (tubeAddress != 0)) /* Deal with Tube output */
  {
    uchar c;

    c = data & 0XFF;

    if (!printOut) {
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
    if ((address >> 2) < memSize) {
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
      printOut = false;
    }

    if ((runFlags & 0x20) &&
        (source == memData)) /* check watchpoints enabled */
    {
      if (checkWatchpoints(address, data, size, 0)) {
        status = CLIENT_STATE_WATCHPOINT;
      }
    }
  }
}

/**
 * @brief
 * @param address
 * @param data
 * @param size
 * @param direction
 * @return int
 */
int checkWatchpoints(uint address, int data, int size, int direction) {
  bool may_break = false;

  for (int i = 0; (i < NO_OF_WATCHPOINTS) && !may_break; i++) {
    may_break = ((emulWPFlag[0] & emulWPFlag[1] & (1 << i)) != 0);
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
          if ((address < watchpoints[i].addrA) ||
              (address > watchpoints[i].addrB))
            may_break = false;
          break;

        case 0x0C: /* Case of mask */
          if ((address & watchpoints[i].addrB) != watchpoints[i].addrA)
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
          if ((data < watchpoints[i].dataA[0]) ||
              (data > watchpoints[i].dataB[0]))
            may_break = false;
          break;

        case 0x03: /* Case of mask */
          if ((data & watchpoints[i].dataB[0]) != watchpoints[i].dataA[0])
            may_break = false;
          break;
      }
  }

  return may_break;
}

/**
 * @brief Get the Number object
 * @param ptr
 * @return int
 */
int getNumber(char* ptr) {
  int a = 0;

  while (*ptr == ' ') {
    ptr++;  // Strip any leading spaces
  }

  while (((*ptr >= '0') && (*ptr <= '9')) || ((*ptr >= 'A') && (*ptr <= 'F'))) {
    if ((*ptr >= '0') && (*ptr <= '9')) {
      a = 16 * a + *(ptr++) - '0';
    } else {
      a = 16 * a + *(ptr++) - 'A' + 10;
    }
  }
  return a;
}

/**
 * @brief
 * @param value
 * @param distance
 * @param cf is -internal- bool
 * @return int
 */
int lsl(int value, int distance, int* cf) {
  int result;

  if (distance != 0) {
    if (distance < 32) {
      result = value << distance;
      *cf = (((value << (distance - 1)) & bit31) != 0);
    } else {
      result = 0X00000000;
      if (distance == 32)
        *cf = ((value & bit0) != 0);
      else
        *cf = (0 != 0); /* internal "false" value */
    }
  } else
    result = value;

  return result;
}

/**
 * @brief
 * @param value
 * @param distance
 * @param cf is -internal- bool
 * @return int
 */
int lsr(uint value, int distance, int* cf) {
  uint result, mask;

  if (distance != 0) {
    if (distance < 32) {
      if (distance != 0)
        mask = ~(0XFFFFFFFF << (32 - distance));
      else
        mask = 0XFFFFFFFF;
      result = (value >> distance) & mask; /* Enforce logical shift */
      *cf = (((value >> (distance - 1)) & bit0) != 0);
    } else { /* Make a special case because C is so crap */
      result = 0X00000000;
      if (distance == 32)
        *cf = ((value & bit31) != 0);
      else
        *cf = (0 != 0); /* internal "false" value */
    }
  } else
    result = value;

  return result;
}

/**
 * @brief
 * @param value
 * @param distance
 * @param cf is -internal- bool
 * @return int
 */
int asr(int value, int distance, int* cf) {
  int result;

  if (distance != 0) {
    if (distance < 32) {
      result = value >> distance;
      if (((value & bit31) != 0) && (distance != 0))
        result = result | (0XFFFFFFFF << (32 - distance));
      /* Sign extend - I don't trust the compiler */
      *cf = (((value >> (distance - 1)) & bit0) != 0);
    } else { /* Make a special case because C is so crap */
      *cf = ((value & bit31) != 0);
      if ((value & bit31) == 0)
        result = 0X00000000;
      else
        result = 0XFFFFFFFF;
    }
  } else
    result = value;

  return result;
}

/**
 * @brief
 * @param value
 * @param distance
 * @param cf is -internal- bool
 * @return int
 */
int ror(uint value, int distance, int* cf) {
  int result;

  if (distance != 0) {
    distance = distance & 0X1F;
    result = lsr(value, distance, cf) | lsl(value, 32 - distance, cf);
    /* cf acts as dummy here */
    *cf = (((value >> (distance - 1)) & bit0) != 0);
  } else
    result = value;

  return result;
}

/**
 * @brief
 * @param op_code
 */
void data0(uint op_code) {
  uint op2, rn;
  uint shift, result;
  int cf;

  rn =
      getRegister(((op_code >> 3) & 7), regCurrent); /* Called "Rm" in shifts */
  shift = ((op_code >> 6) & 0X0000001F); /* Extracted speculatively */

  if ((op_code & 0X1800) != 0X1800) /* Shifts */
  {
    cf = ((cpsr & cfMask) != 0); /* default */
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
      cpsr = cpsr | cfMask;
    else
      cpsr = cpsr & ~cfMask;
    setNZ(result);
    putRegister((op_code & 7), result, regCurrent);
  } else {
    if ((op_code & 0X0400) == 0) /* ADD(3)/SUB(3) */
    {
      op2 = getRegister((op_code >> 6) & 7, regCurrent);
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

    putRegister(op_code & 7, result, regCurrent);
  }
}

/**
 * @brief
 * @param op_code
 */
void data1(uint op_code) {
  int rd, imm;
  int result;

  rd = (op_code >> 8) & 7;
  imm = op_code & 0X00FF;

  switch (op_code & 0X1800) {
    case 0X0000: /* MOV (1) */
      result = imm;
      setNZ(result);
      putRegister(rd, result, regCurrent);
      break;

    case 0X0800: /* CMP (1) */
      result = (getRegister(rd, regCurrent) - imm);
      setFlags(flagSub, getRegister(rd, regCurrent), imm, result, 1);
      break;

    case 0X1000: /* ADD (2) */
      result = (getRegister(rd, regCurrent) + imm);
      setFlags(flagAdd, getRegister(rd, regCurrent), imm, result, 0);
      putRegister(rd, result, regCurrent);
      break;

    case 0X1800: /* SUB (2) */
      result = (getRegister(rd, regCurrent) - imm);
      setFlags(flagSub, getRegister(rd, regCurrent), imm, result, 1);
      putRegister(rd, result, regCurrent);
      break;
  }
}

/**
 * @brief
 * @param op_code
 */
void dataTransfer(uint op_code) {
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
        rd = getRegister((op_code & 7), regCurrent);
        rm = getRegister(((op_code >> 3) & 7), regCurrent);

        switch (op_code & 0X03C0) /* data processing opcode */
        {
          case 0X0000: /* AND */
            result = rd & rm;
            putRegister(op_code & 7, result, regCurrent);
            setNZ(result);
            break;

          case 0X0040: /* EOR */
            result = rd ^ rm;
            putRegister(op_code & 7, result, regCurrent);
            setNZ(result);
            break;

          case 0X0080:                   /* LSL (2) */
            cf = ((cpsr & cfMask) != 0); /* default */
            result = lsl(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cfMask;
            else
              cpsr = cpsr & ~cfMask;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X00C0:                   /* LSR (2) */
            cf = ((cpsr & cfMask) != 0); /* default */
            result = lsr(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cfMask;
            else
              cpsr = cpsr & ~cfMask;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X0100:                   /* ASR (2) */
            cf = ((cpsr & cfMask) != 0); /* default */
            result = asr(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cfMask;
            else
              cpsr = cpsr & ~cfMask;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X0140: /* ADC */
            result = rd + rm;
            if ((cpsr & cfMask) != 0)
              result = result + 1; /* Add CF */
            setFlags(flagAdd, rd, rm, result, cpsr & cfMask);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X0180: /* SBC */
            result = rd - rm - 1;
            if ((cpsr & cfMask) != 0)
              result = result + 1;
            setFlags(flagSub, rd, rm, result, cpsr & cfMask);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X01C0:                   /* ROR */
            cf = ((cpsr & cfMask) != 0); /* default */
            result = ror(rd, rm & 0X000000FF, &cf);
            if (cf)
              cpsr = cpsr | cfMask;
            else
              cpsr = cpsr & ~cfMask;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X0200: /* TST */
            setNZ(rd & rm);
            break;

          case 0X0240: /* NEG */
            result = -rm;
            putRegister(op_code & 7, result, regCurrent);
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
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X00340: /* MUL */
            result = rm * rd;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X0380: /* BIC */
            result = rd & ~rm;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;

          case 0X03C0: /* MVN */
            result = ~rm;
            setNZ(result);
            putRegister(op_code & 7, result, regCurrent);
            break;
        }    /* End of switch */
      } else /* special data processing */
      {      /* NO FLAG UPDATE */
        switch (op_code & 0X0300) {
          case 0X0000: /* ADD (4) high registers */
            rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
            rm = getRegister(((op_code >> 3) & 15), regCurrent);
            putRegister(rd, getRegister(rd, regCurrent) + rm, regCurrent);
            break;

          case 0X0100: /* CMP (3) high registers */
            rd = getRegister((((op_code & 0X0080) >> 4) | (op_code & 7)),
                             regCurrent);
            rm = getRegister(((op_code >> 3) & 15), regCurrent);
            setFlags(flagSub, rd, rm, rd - rm, 1);
            break;

          case 0X0200: /* MOV (2) high registers */
            rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
            rm = getRegister(((op_code >> 3) & 15), regCurrent);

            if (rd == 15)
              rm = rm & 0XFFFFFFFE; /* Tweak mov to PC */
            putRegister(rd, rm, regCurrent);
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
                (getRegister(15, regCurrent) & 0XFFFFFFFC);
      putRegister(rd, readMemory(address, 4, false, false, memData),
                  regCurrent);
    }
  } else { /* load/store word, halfword, byte, signed byte */
    int rm, rn;
    int data;

    rd = (op_code & 7);
    rn = getRegister(((op_code >> 3) & 7), regCurrent);
    rm = getRegister(((op_code >> 6) & 7), regCurrent);

    switch (op_code & 0X0E00) {
      case 0X0000: /* STR (2) register */
        writeMemory(rn + rm, getRegister(rd, regCurrent), 4, false, memData);
        break;

      case 0X0200: /* STRH (2) register */
        writeMemory(rn + rm, getRegister(rd, regCurrent), 2, false, memData);
        break;

      case 0X0400: /* STRB (2) register */
        writeMemory(rn + rm, getRegister(rd, regCurrent), 1, false, memData);
        break;

      case 0X0600: /* LDRSB register */
        data = readMemory(rn + rm, 1, true, false, memData); /* Sign ext. */
        putRegister(rd, data, regCurrent);
        break;

      case 0X0800: /* LDR (2) register */
        data = readMemory(rn + rm, 4, false, false, memData);
        putRegister(rd, data, regCurrent);
        break;

      case 0X0A00: /* LDRH (2) register */
        data = readMemory(rn + rm, 2, false, false, memData); /* Zero ext. */
        putRegister(rd, data, regCurrent);
        break;

      case 0X0C00:                                            /* LDRB (2) */
        data = readMemory(rn + rm, 1, false, false, memData); /* Zero ext. */
        putRegister(rd, data, regCurrent);
        break;

      case 0X0E00:                                           /* LDRSH (2) */
        data = readMemory(rn + rm, 2, true, false, memData); /* Sign ext. */
        putRegister(rd, data, regCurrent);
        break;
    }
  }
}

/**
 * @brief
 * @param op_code
 */
void transfer0(uint op_code) {
  int rd, rn;
  int location, data;

  rn = getRegister(((op_code >> 3) & 7), regCurrent);

  if ((op_code & 0X0800) == 0) /* STR */
  {
    rd = getRegister((op_code & 7), regCurrent);
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
    putRegister(rd, data, regCurrent);
  }
}

/**
 * @brief
 * @param op_code
 */
void transfer1(uint op_code) {
  int rd, rn;
  int data, location;

  switch (op_code & 0X1800) {
    case 0X0000: /* STRH (1) */
      rn = getRegister((op_code >> 3) & 7, regCurrent);
      rd = op_code & 7;
      data = getRegister(rd, regCurrent);
      location = rn + ((op_code >> 5) & 0X3E); /* x2 in shift */
      writeMemory(location, data, 2, false, memData);
      break;

    case 0X0800: /* LDRH (1) */
      rd = op_code & 7;
      rn = getRegister((op_code >> 3) & 7, regCurrent);
      location = rn + ((op_code >> 5) & 0X3E);               /* x2 in shift */
      data = readMemory(location, 2, false, false, memData); /* Zero extended */
      putRegister(rd, data, regCurrent);
      break;

    case 0X1000: /* STR (3) -SP */
      data = getRegister(((op_code >> 8) & 7), regCurrent);
      rn = getRegister(13, regCurrent); /* SP */
      location = rn + ((op_code & 0X00FF) * 4);
      writeMemory(location, data, 4, false, memData);
      break;

    case 0X1800: /* LDR (4) -SP */
      rd = (op_code >> 8) & 7;
      rn = getRegister(13, regCurrent);         /* SP */
      location = rn + ((op_code & 0X00FF) * 4); /* x2 in shift */
      data = readMemory(location, 4, false, false, memData);
      putRegister(rd, data, regCurrent);
      break;
  }
}

/**
 * @brief
 * @param op_code
 */
void spPC(uint op_code) {
  int rd, sp, data;

  if ((op_code & 0X1000) == 0) /* ADD SP or PC */
  {
    rd = (op_code >> 8) & 7;

    if ((op_code & 0X0800) == 0) /* ADD(5) -PC */
      data = (getRegister(15, regCurrent) & 0XFFFFFFFC) +
             ((op_code & 0X00FF) << 2);
    /* getRegister supplies PC + 2 */
    else /* ADD(6) -SP */
      data = (getRegister(13, regCurrent)) + ((op_code & 0X00FF) << 2);
    putRegister(rd, data, regCurrent);
  } else /* Adjust SP */
  {
    switch (op_code & 0X0F00) {
      case 0X0000:
        if ((op_code & 0X0080) == 0) /* ADD(7) -SP */
          sp = getRegister(13, regCurrent) + ((op_code & 0X7F) << 2);
        else /* SUB(4) -SP */
          sp = getRegister(13, regCurrent) - ((op_code & 0X7F) << 2);
        putRegister(13, sp, regCurrent);
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

/**
 * @brief
 * @param op_code
 */
void lsmB(uint op_code) {
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

        putRegister(15, getRegister(15, regCurrent) + offset, regCurrent);
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

/**
 * @brief
 * @param op_code
 * @param exchange
 */
void thumbBranch1(uint op_code, int exchange) {
  int offset, lr;

  lr = getRegister(14, regCurrent); /* Retrieve first part of offset */
  offset = lr + ((op_code & 0X07FF) << 1);

  lr = getRegister(15, regCurrent) - 2 + 1; /* + 1 to indicate Thumb mode */

  if (exchange == true) {
    cpsr = cpsr & ~tfMask; /* Change to ARM mode */
    offset = offset & 0XFFFFFFFC;
  }

  putRegister(15, offset, regCurrent);
  putRegister(14, lr, regCurrent);
}

/**
 * @brief
 * @param op_code
 */
void thumbBranch(uint op_code) {
  int offset;

  switch (op_code & 0X1800) {
    case 0X0000: /* B -uncond. B(2)  */
      offset = (op_code & 0X07FF) << 1;
      if ((op_code & 0X0400) != 0)
        offset = offset | 0XFFFFF000; /* sign extend */
      putRegister(15, (getRegister(15, regCurrent) + offset), regCurrent);
      break;

    case 0X0800: /* BLX */
      if ((op_code & 0X0001) == 0)
        thumbBranch1(op_code, true);
      else
        fprintf(stderr, "Undefined\n");
      break;

    case 0X1000: /* BL prefix */
      BLPrefix = op_code & 0X07FF;
      offset = BLPrefix << 12;

      if ((BLPrefix & 0X0400) != 0)
        offset = offset | 0XFF800000; /* Sign ext. */
      offset = getRegister(15, regCurrent) + offset;
      putRegister(14, offset, regCurrent);
      break;

    case 0X1800:
      thumbBranch1(op_code, false);
      break;
  }
}

/**
 * @brief
 * @param number
 * @return uint
 */
uint getmem32(int number) {
  number = number % RAMSIZE;
  return memory[(number << 2)] | memory[(number << 2) + 1] << 8 |
         memory[(number << 2) + 2] << 16 | memory[(number << 2) + 3] << 24;
}

/**
 * @brief
 * @param number
 * @param reg
 */
void setmem32(int number, uint reg) {
  number = number & (RAMSIZE - 1);
  memory[(number << 2) + 0] = (reg >> 0) & 0xff;
  memory[(number << 2) + 1] = (reg >> 8) & 0xff;
  memory[(number << 2) + 2] = (reg >> 16) & 0xff;
  memory[(number << 2) + 3] = (reg >> 24) & 0xff;
}

/**
 * @brief
 * @param buffer
 */
void initBuffer(ringBuffer* buffer) {
  buffer->iHead = 0;
  buffer->iTail = 0;
}

/**
 * @brief Measure buffer occupancy.
 * @param buffer
 * @return int
 */
int countBuffer(ringBuffer* buffer) {
  int i = buffer->iHead - buffer->iTail;

  if (i < 0) {
    i = i + RING_BUF_SIZE;
  }
  return i;
}

/**
 * @brief
 * @param buffer
 * @param c
 * @return int
 */
bool putBuffer(ringBuffer* buffer, const uchar c) {
  uint temp = (buffer->iHead + 1) % RING_BUF_SIZE;

  if (temp != buffer->iTail) {
    buffer->buffer[temp] = c;
    buffer->iHead = temp;
    return true;
  }

  return false;
}

/**
 * @brief Get the Buffer object
 * @param buffer
 * @param c
 * @return int
 */
bool getBuffer(ringBuffer* buffer, uchar* c) {
  if (buffer->iTail != buffer->iHead) {
    buffer->iTail = (buffer->iTail + 1) % RING_BUF_SIZE;
    *c = buffer->buffer[buffer->iTail];
    return true;
  }

  return false;
}
