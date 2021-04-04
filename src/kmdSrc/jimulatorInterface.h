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
const int checkBoardState();
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
