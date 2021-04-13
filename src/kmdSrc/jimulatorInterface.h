/**
 * @file jimulatorInterface.h
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The header file associated with the `jimulatorInterface.c` file -
 * specifies functions which can be accessed externally to by other files which
 * include this header.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <array>
#include <string>

constexpr int SOURCE_BYTE_COUNT = 4;
constexpr int SOURCE_FIELD_COUNT = 4;
constexpr int SOURCE_TEXT_LENGTH = 100;
constexpr int IN_POLL_TIMEOUT = 1000;
constexpr int OUT_POLL_TIMEOUT = 100;
constexpr int MAX_SERIAL_WORD = 4;
constexpr int ADDRESS_BUS_WIDTH = 4;
constexpr int MAX_NUMBER_OF_BREAKPOINTS = 32;

/**
 * @brief A series of values that represent state information returned from
 * Jimulator.
 */
enum class ClientState : unsigned char {
  NORMAL = 0X00,
  BUSY = 0X01,
  BREAKPOINT = 0X41,
  MEMFAULT = 0X43,
  FINISHED = 0X44,
  RUNNING = 0X80,
  RUNNING_SWI = 0x81,
  STEPPING = 0X82,
  BROKEN = 0x30,
};

/**
 * @brief Performing an or between a ClientState and an unsigned char.
 * @param l The left hand ClientState value.
 * @param r The right hand unsigned char value.
 * @return unsigned char The result of the or operation.
 */
inline unsigned char operator|(ClientState l, unsigned char r) {
  return static_cast<unsigned char>(l) | r;
}

/**
 * @brief Contains the information read from Jimulator about a given breakpoint.
 */
class BreakpointInfo {
 public:
  unsigned char addressA[ADDRESS_BUS_WIDTH];
  unsigned char addressB[ADDRESS_BUS_WIDTH] = {0xFF, 0XFF, 0XFF, 0XFF};
  unsigned char dataA[8] = {0};
  unsigned char dataB[8] = {0};
  unsigned int misc = 0xFFFFFFFF;
};

/**
 * @brief A container for a series of codes used as board instructions.
 */
enum class BoardInstruction : unsigned char {
  // General commands
  START = 0xB0,
  WOT_U_DO = 0x20,
  STOP = 0x21,
  CONTINUE = 0x23,
  RESET = 0x04,

  // Terminal read/write
  FR_WRITE = 0x12,
  FR_READ = 0x13,

  // Breakpoint read/write
  BP_WRITE = 0x30,
  BP_READ = 0x31,
  BP_SET = 0x32,
  BP_GET = 0x33,

  // Register read/write
  GET_REG = 0x5A,
  SET_REG = 0x52,  // Unused

  // Memory read/write
  GET_MEM = 0x4A,
  SET_MEM = 0x40,
};

/**
 * @brief Performing an or between a BoardInstruction and an unsigned char.
 * @param l The left hand BoardInstruction value.
 * @param r The right hand unsigned char value.
 * @return unsigned char The result of the or operation.
 */
inline unsigned char operator|(BoardInstruction l, unsigned char r) {
  return static_cast<unsigned char>(l) | r;
}

/**
 * @brief Describes a single line of a .kmd file.
 */
class SourceFileLine {
 public:
  SourceFileLine* prev;  // Previous line
  SourceFileLine* next;  // Next line
  bool hasData;          // Flag if line has no data fields
  unsigned int address;  // Address of entry
  int dataSize[4];       // Sizes of fields
  int dataValue[4];      // Data values
  char* text;            // Text, as imported
};

/**
 * @brief Describes an entire file of a .kmd sourceFile.
 */
class sourceFile {
 public:
  SourceFileLine* pStart;  // First line in source (sorted into address order)
  SourceFileLine* pEnd;    // Last line in source
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
const ClientState checkBoardState();
const std::array<std::string, 16> getJimulatorRegisterValues();
std::array<Jimulator::MemoryValues, 13> getJimulatorMemoryValues(
    const uint32_t s_address_int);
const std::string getJimulatorTerminalMessages();

// ! Loading data
void compileJimulator(const char* const pathToBin,
                      const char* const pathToS,
                      const char* const pathToKMD);
const bool loadJimulator(const char* const pathToKMD);

// ! Sending commands
void startJimulator(const int steps);
void continueJimulator();
void pauseJimulator();
void resetJimulator();
const bool sendTerminalInputToJimulator(const unsigned int val);
const bool setBreakpoint(const uint32_t address);
}  // namespace Jimulator
