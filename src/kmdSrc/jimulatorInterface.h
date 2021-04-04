/**
 * @file jimulatorInterface.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The header file associated with the `jimulatorInterface.c` file -
 * specifies functions which can be accessed externally to by other files which
 * include this header.
 * @version 0.1
 * @date 2020-11-27
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 */

#include <array>
#include <string>

const int SOURCE_BYTE_COUNT = 4;
const int SOURCE_FIELD_COUNT = 4;
const int SOURCE_TEXT_LENGTH = 100;
const int SYM_BUF_SIZE = 64;
const int IN_POLL_TIMEOUT = 1000;
const int OUT_POLL_TIMEOUT = 100;
const int MAX_SERIAL_WORD = 4;

enum class clientState {
  CLASS_MASK = 0XC0,
  NORMAL = 0X00,
  CLASS_STOPPED = 0X40,
  CLASS_RUNNING = 0X80,
  CLASS_ERROR = 0XC0,
  RESET = 0X00,
  BUSY = 0X01,
  STOPPED = 0X40,
  BREAKPOINT = 0X41,
  WATCHPOINT = 0X42,
  MEMFAULT = 0X43,
  PROG_FINISHED = 0X44,  // Program ended
  RUNNING = 0X80,        // Program running
  RUNNING_BL = 0x81,
  RUNNING_SWI = 0x81,
  STEPPING = 0X82,
  BROKEN = 0x30,
};

enum class runFlags {
  BL = 0x02,
  SWI = 0x04,
  ABORT = 0x08,
  BREAK = 0x10,
  WATCH = 0x20,
  INIT = 0x30,
};

class breakpointInfo {
 public:
  unsigned int misc;
  unsigned char addressA[8];
  unsigned char addressB[8];
  unsigned char dataA[8];
  unsigned char dataB[8];
};

/**
 * @brief A container for a series of codes used as board instructions.
 */
enum class boardInstruction : unsigned char {
  NOP = 0x00,
  PING = 0x01,
  WOT_R_U = 0x02,
  RESET = 0x04,
  COMM_W = 0x06,
  COMM_R = 0x07,

  FR_GET = 0x10,
  FR_SET = 0x11,
  FR_WRITE = 0x12,
  FR_READ = 0x13,
  FR_FILE = 0x14,
  FR_SEND = 0x15,

  WOT_U_DO = 0x20,
  STOP = 0x21,
  PAUSE = 0x22,
  CONTINUE = 0x23,

  RTF_SET = 0x24,
  RTF_GET = 0x25,

  BP_WRITE = 0x30,
  BP_READ = 0x31,
  BP_SET = 0x32,
  BP_GET = 0x33,

  GET_REG = 0x5A,  // not general case!
  GET_MEM = 0x48,
  SET_REG = 0x52,  // not general case!
  SET_MEM = 0x40,

  WP_WRITE = 0x34,
  WP_READ = 0x35,
  WP_SET = 0x36,
  WP_GET = 0x37,

  START = 0x80
};

/**
 * @brief An imported source code (listing) line
 */
class sourceFileLine {
 public:
  sourceFileLine* pPrev;  // Previous line
  sourceFileLine* pNext;  // Next line
  int corrupt;            // Flag if value changed
  int nodata;             // Flag if line has no data fields
  unsigned int address;   // Address of entry
  int data_size[4];       // Sizes of fields
  int data_value[4];      // Data values
  char* text;             // Text, as imported
};

class sourceFile {
 public:
  sourceFileLine* pStart;  // First line in source (sorted into address order)
  sourceFileLine* pEnd;    // Last line in source
};

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

/**
 * @brief This namespace groups together regular non-member functions such that,
 * when called from other places in the codebase, it is clear where the
 * functions are being called from.
 */
namespace Jimulator {
/**
 * @brief A class that returns all of the information associated with a single
 * row of a memory window, as read from Jimulator.
 */
class MemoryValues {
 public:
  /**
   * @brief The address of the memory value.
   */
  u_int32_t address;
  /**
   * @brief A hexadecimal representation of what is stored in that memory value.
   */
  std::string hex;
  /**
   * @brief What the actual .s file says on this line.
   */
  std::string disassembly;
  /**
   * @brief Whether or not a breakpoint is set for this address.
   */
  bool breakpoint = false;
};

// ! Reading data
const clientState checkBoardState();
const std::array<std::string, 16> getJimulatorRegisterValues();
std::array<Jimulator::MemoryValues, 15> getJimulatorMemoryValues(
    const uint32_t s_address_int);
const std::string getJimulatorTerminalMessages();

// ! Loading data
const int compileJimulator(const char* const pathToBin,
                           const char* const pathToS,
                           const char* const pathToKMD);
const int loadJimulator(const char* const pathToKMD);

// ! Sending commands
void startJimulator(const int steps);
void continueJimulator();
void pauseJimulator();
void resetJimulator();
const bool sendTerminalInputToJimulator(const unsigned int val);
const bool setBreakpoint(uint32_t address);
}  // namespace Jimulator
