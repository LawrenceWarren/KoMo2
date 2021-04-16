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
 * @brief Groups together functions that make up the Jimulator API layer - these
 * functions and classes are used for sending and receiving information from
 * the emulator.
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
