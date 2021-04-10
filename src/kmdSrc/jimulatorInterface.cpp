/**
 * @file jimulatorInterface.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Contains functionality relating to the serialization, transmission and
 * reception of data to and from Jimulator, with a minor amount of processing
 * done either way.
 * @version 1.0.0
 * @date 10-04-2021
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

#include "jimulatorInterface.h"
#include <ctype.h>
#include <fcntl.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

// ! Forward declaring auxiliary load functions

// Workers

void flushSourceFile();
const bool readSourceFile(const char* const);
const ClientState getBoardStatus();
const std::array<unsigned char, 64> readRegistersIntoArray();
constexpr const int disassembleSourceFile(SourceFileLine*, unsigned int);

// Low level sending

void boardSendNBytes(int, int);
void boardSendChar(unsigned char);
void boardSendCharArray(int, unsigned char*);

// Low level receiving

const int boardGetNBytes(int*, int);
const int boardGetChar(unsigned char*);
const int boardGetCharArray(int, unsigned char*);

// Breakpoints

constexpr const int getNextFreeBreakpoint(const int);
const bool getBreakpointStatus(unsigned int*, unsigned int*);
const bool getBreakpointDefinition(unsigned int, BreakpointInfo*);
void setBreakpointStatus(unsigned int, unsigned int);
void setBreakpointDefinition(unsigned int, BreakpointInfo*);
const std::unordered_map<u_int32_t, bool> getAllBreakpoints();

// Helpers

constexpr void copyStringLiterals(int, unsigned char*, unsigned char*);
constexpr const int numericStringSubtraction(const unsigned char* const,
                                             const unsigned char* const);
constexpr const int numericStringToInt(int, const unsigned char* const);
constexpr void numericStringAndIntAddition(unsigned char* const, int);
const std::string integerArrayToHexString(int,
                                          unsigned char* const,
                                          const bool = false);
constexpr const char getLeastSignificantByte(const int);

// The read .kmd file
sourceFile source;

/**
 * @brief Runs `pathToS` through the associated compiler binary, and outputs a
 * .kmd file at `pathToKMD`.
 * @param pathToBin An absolute path to the `aasm` binary.
 * @param pathToS An absolute path to the `.s` file to be compiled.
 * @param pathToKMD an absolute path to the `.kmd` file that will be output.
 */
void Jimulator::compileJimulator(const char* const pathToBin,
                                 const char* const pathToS,
                                 const char* const pathToKMD) {
  close(1);
  dup2(compilerCommunication[1], 1);
  close(2);
  dup2(compilerCommunication[1], 2);
  execlp(pathToBin, "aasm", "-lk", pathToKMD, pathToS, (char*)0);
}

/**
 * @brief Clears the existing `source` object and loads the file at `pathToKMD`
 * into Jimulator.
 * @param pathToKMD an absolute path to the `.kmd` file that will be loaded.
 * @returns
 */
const bool Jimulator::loadJimulator(const char* const pathToKMD) {
  flushSourceFile();
  return readSourceFile(pathToKMD);
}

/**
 * @brief Commences running the emulator.
 * @param steps The number of steps to run for (0 for indefinite)
 */
void Jimulator::startJimulator(const int steps) {
  if (checkBoardState() == ClientState::NORMAL ||
      Jimulator::checkBoardState() == ClientState::BREAKPOINT) {
    boardSendChar(static_cast<unsigned char>(BoardInstruction::START));
    boardSendNBytes(steps, 4);  // Send step count
  }
}

/**
 * @brief Continues running Jimulator.
 */
void Jimulator::continueJimulator() {
  if (Jimulator::checkBoardState() == ClientState::NORMAL ||
      Jimulator::checkBoardState() == ClientState::BREAKPOINT) {
    boardSendChar(static_cast<unsigned char>(BoardInstruction::CONTINUE));
  }
}

/**
 * @brief Pauses the emulator running.
 */
void Jimulator::pauseJimulator() {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::STOP));
}

/**
 * @brief Reset the emulators running.
 */
void Jimulator::resetJimulator() {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::RESET));
}

/**
 * @brief Sets a breakpoint.
 * @param addr The address to set the breakpoint at.
 * @return const bool If setting the breakpoint succeeded.
 */
const bool Jimulator::setBreakpoint(const uint32_t addr) {
  unsigned int wordA = 0, wordB = 0;
  unsigned char address[ADDRESS_BUS_WIDTH] = {0};

  // Unpack address to byte array
  for (int i = 0; i < ADDRESS_BUS_WIDTH; i++) {
    address[i] = getLeastSignificantByte(addr >> (8 * i));
  }

  // Reads the breakpoints
  if (not getBreakpointStatus(&wordA, &wordB)) {
    return false;
  }

  // Checks to see if a breakpoint exists at this address and turns it off if so
  for (int i = 0; i < MAX_NUMBER_OF_BREAKPOINTS; i++) {
    if (((wordA >> i) & 1) != 0) {
      BreakpointInfo bp;

      // Gets breakpoint i from the list;
      // checks if that breakpoint is set for current address;
      // turns it off if so
      if (getBreakpointDefinition(i, &bp) &&
          (numericStringSubtraction(address, bp.addressA) == 0)) {
        setBreakpointStatus(0, 1 << i);
        return false;
      }
    }
  }

  // See if there are any more breakpoints to be set, return if not
  int temp = (~wordA) & wordB;
  if (temp == 0) {
    return false;
  }

  // Set the breakpoint
  BreakpointInfo bp;
  copyStringLiterals(ADDRESS_BUS_WIDTH, address, bp.addressA);

  int i = getNextFreeBreakpoint(temp);
  setBreakpointDefinition(i, &bp);
  return true;
}

/**
 * @brief Check the state of the board - logs what it is doing.
 * @return int 0 if the board is in a failed state, else a number greater
 * than 0.
 */
const ClientState Jimulator::checkBoardState() {
  const auto board_state = getBoardStatus();

  // Check and log error states
  switch (board_state) {
    case ClientState::RUNNING_SWI:
      break;
    case ClientState::RUNNING:
      break;
    case ClientState::STEPPING:
      break;
    case ClientState::MEMFAULT:
      break;
    case ClientState::BUSY:
      break;
    case ClientState::FINISHED:
      break;
    case ClientState::BREAKPOINT:
      break;
    default:
      return ClientState::NORMAL;
      break;
  }

  return board_state;
}

/**
 * @brief Queries a register in Jimulator to get it's current value.
 * @return The values read from the registers.
 */
const std::array<std::string, 16> Jimulator::getJimulatorRegisterValues() {
  auto bytes = readRegistersIntoArray();

  std::array<std::string, 16> ret;  // vector of strings

  // Loop through the array
  for (long unsigned int i = 0; i < ret.size(); i++) {
    ret[i] = integerArrayToHexString(4, &bytes[i * 4], true);
  }

  return ret;
}

/**
 * @brief Reads for messages from Jimulator, to display in the terminal output.
 * @return const std::string The message to be displayed in the terminal output.
 */
const std::string Jimulator::getJimulatorTerminalMessages() {
  unsigned char string[256];  // Arbitrary size
  unsigned char length = 1;

  std::string output("");

  while (length > 0) {
    boardSendChar(static_cast<unsigned char>(BoardInstruction::FR_READ));
    boardSendChar(0);  // send the terminal number
    boardSendChar(32);
    boardGetChar(&length);  // get length of message

    // non-zero received from board - not an empty packet
    if (length != 0) {
      boardGetCharArray(length, string);  // Store the message
      string[length] = '\0';
      output.append((char*)string);
    }
  }

  return output;
}

/**
 * @brief Sends terminal information to Jimulator.
 * @param val A key code.
 * @return true If the key was sent to Jimulator successfully.
 * @return false If the key was not sent to Jimulator successfully.
 */
const bool Jimulator::sendTerminalInputToJimulator(const unsigned int val) {
  unsigned int key_pressed = val;
  unsigned char res = 0;

  // Translate key codes if necessary and understood
  switch (key_pressed) {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      key_pressed = '\n';
      break;
    case GDK_KEY_BackSpace:
      key_pressed = '\b';
      break;
    case GDK_KEY_Tab:
      key_pressed = '\t';
      break;
    case GDK_KEY_Escape:
      key_pressed = 0x1B;
      break;
    case GDK_KEY_KP_0:
    case GDK_KEY_KP_1:
    case GDK_KEY_KP_2:
    case GDK_KEY_KP_3:
    case GDK_KEY_KP_4:
    case GDK_KEY_KP_5:
    case GDK_KEY_KP_6:
    case GDK_KEY_KP_7:
    case GDK_KEY_KP_8:
    case GDK_KEY_KP_9:
      key_pressed = key_pressed - GDK_KEY_KP_0 + '0';
      break;
    case GDK_KEY_KP_Add:
      key_pressed = '+';
      break;
    case GDK_KEY_KP_Subtract:
      key_pressed = '-';
      break;
    case GDK_KEY_KP_Multiply:
      key_pressed = '*';
      break;
    case GDK_KEY_KP_Divide:
      key_pressed = '/';
      break;
    case GDK_KEY_KP_Decimal:
      key_pressed = '.';
      break;
    default:
      break;
  }

  // Sending keys to Jimulator
  if (((key_pressed >= ' ') && (key_pressed <= 0x7F)) ||
      (key_pressed == '\n') || (key_pressed == '\b') || (key_pressed == '\t') ||
      (key_pressed == '\a')) {
    boardSendChar(static_cast<unsigned char>(
        BoardInstruction::FR_WRITE));  // begins a write
    boardSendChar(0);                  // tells where to send it
    boardSendChar(1);                  // send length 1
    boardSendChar(key_pressed);        // send the message - 1 char currently
    boardGetChar(&res);                // Read the result
    return true;
  }

  return false;
}

/**
 * @brief Get the memory values from Jimulator, starting to s_address.
 * @param s_address The address to start at, as an integer.
 * @return std::array<Jimulator::MemoryValues, 13> An array of all of the
 * values read from Jimulator, including each column.
 */
std::array<Jimulator::MemoryValues, 13> Jimulator::getJimulatorMemoryValues(
    const uint32_t s_address) {
  const int count = 13;  // The number of values displayed in the memory window
  const int bytecount = count * ADDRESS_BUS_WIDTH;  // How many bytes to read

  // Bit level hacking happening here - converting the integer address into
  // an array of characters.
  unsigned char* p = (unsigned char*)&s_address;
  unsigned char currentAddressS[ADDRESS_BUS_WIDTH] = {p[0], p[1], p[2], p[3]};
  currentAddressS[0] &= -4;  // Normalise address down

  // Reading data into arrays!
  unsigned char memdata[bytecount];
  boardSendChar(static_cast<unsigned char>(BoardInstruction::GET_MEM));
  boardSendCharArray(ADDRESS_BUS_WIDTH, currentAddressS);
  boardSendNBytes(count, 2);
  boardGetCharArray(bytecount, memdata);

  // ! Dangerous old logic ahead is used to read memory.
  // ! This is very much C-style code, be careful

  SourceFileLine* src = NULL;
  bool used_first = false;

  // Moves our src line to the relevant line of the src file
  if (source.pStart != NULL) {
    src = source.pStart;  // Known to be valid
    while ((src != NULL) && ((src->address < s_address) || not src->hasData)) {
      src = src->next;
    }

    // We fell off the end; wrap to start
    if (src == NULL) {
      src = source.pStart;
      used_first = true;

      // Find a record with some data
      while ((src != NULL) && not src->hasData) {
        src = src->next;
      }
    }
  }

  // Data is read into this array
  std::array<Jimulator::MemoryValues, 13> readValues;
  const auto bps = getAllBreakpoints();

  // Iterate over display rows
  for (long unsigned int i = 0; i < readValues.size(); i++) {
    int increment = 0;
    unsigned int currentAddressI =
        numericStringToInt(ADDRESS_BUS_WIDTH, currentAddressS);
    readValues[i].address = currentAddressI;
    readValues[i].hex = std::string("00000000");
    readValues[i].disassembly = std::string("...");

    // Generate the hex
    if (src != NULL && currentAddressI == src->address) {
      readValues[i].disassembly =
          std::regex_replace(std::string(src->text), std::regex(";.*$"), "");

      std::string hex;

      // Source field entries
      for (int i = 0; i < SOURCE_FIELD_COUNT; i++) {
        if (src->dataSize[i] > 0) {
          char spaces[5];

          auto data = integerArrayToHexString(
              src->dataSize[i],
              &memdata[currentAddressI - s_address + increment]);

          int j = 0;
          for (; j < src->dataSize[i]; j++) {
            spaces[j] = ' ';
          }

          spaces[j] = '\0';
          hex += data;
          hex += spaces;
        }
        increment = increment + src->dataSize[i];
      }

      readValues[i].hex = hex;

      // step the src file over
      do {
        if (src->next != NULL) {
          src = src->next;  // Move on ...
        } else {            // ... wrapping, if required
          if (not used_first) {
            src = source.pStart;
            used_first = true;
          } else {
            src = NULL;  // Been here before - give in
          }
        }
      } while ((src != NULL) && not src->hasData);
    }

    // Find if a breakpoint is set on this line
    // if the iterator is not at the end of the map, the breakpoint was found
    // and can be set
    auto iter = bps.find(readValues[i].address);
    if (iter != bps.end()) {
      readValues[i].breakpoint = true;
    }

    // Calculate where the next address is and move the address there
    increment = disassembleSourceFile(src, currentAddressI);
    numericStringAndIntAddition(currentAddressS, increment);
  }

  return readValues;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!! Functions below are not included in the header file !!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

/**
 * @brief Gets the index of the next free breakpoint from Jimulators internal
 * breakpoint list.
 * @param v Indicates which breakpoints are set, as an array of bits.
 * @return const int The index of the next free breakpoint.
 */
constexpr const int getNextFreeBreakpoint(const int v) {
  int i = 0;
  for (; (((v >> i) & 1) == 0); i++)
    ;

  return i;
}

/**
 * @brief Get the least significant byte out of an integer - if little endian,
 * get the most signficant byte.
 * @param val The integer to get the least significant byte of.
 * @return const char The least significant byte.
 */
constexpr const char getLeastSignificantByte(const int val) {
  return val & 0xFF;
}

/**
 * @brief Rotates an integers bits to the right by 1 byte.
 * @param val The integer to rotate.
 * @return const int The rotated integer.
 */
constexpr const int rotateRight1Byte(const int val) {
  return val >> 8;
}

/**
 * @brief Rotates an integers bits to the left by 1 byte.
 * @param val The integer to rotate.
 * @return const int The rotated integer.
 */
constexpr const int rotateLeft1Byte(const int val) {
  return val << 8;
}

/**
 * @brief Gets the status of breakpoints from within Jimulator.
 * @warning The internals of Jimulator as a binary are a mystery to me. I have
 * managed to decipher what this function does through trial and error. However
 * I cannot decipher why it works in a (somewhat) mysterious way. This function
 * works in tandem with other functions related to setting and reading
 * breakpoints, and I would avoid changing it without serious understanding of
 * how else it is used and what it does first.
 * @param wordA The first chunk of read breakpoint information will be read into
 * here. It reveals the number of breakpoints in an unusual pattern:
 * - `0` for 0 breakpoints
 * - `1` for 1 breakpoint
 * - `3` for 2 breakpoints
 * - `7` for 3 breakpoints
 * - `15` for 4 breakpoints
 * - `31` for 5 breakpoints
 * - `63`, `127` and so on...
 *
 * The pattern here is, if the number of breakpoints is 'n', it will return a
 * 32-bit integer with the least significant 'n' bits set:
 * - 0b00000000000000000000000000000000 = 0 breakpoints set = 0d0
 * - 0b00000000000000000000000000000001 = 1 breakpoint set  = 0d1
 * - 0b00000000000000000000000000000011 = 2 breakpoints set = 0d3
 * - 0b00000000000000000000000000000111 = 3 breakpoints set = 0d7
 * - 0b00000000000000000000000000001111 = 4 breakpoints set = 0d15
 *
 * This is why there can only be 32 breakpoints total.
 *
 * AGAIN, why this is the way Jimulator works is a mystery.
 * @param wordB The second chunk of read breakpoint information will be read
 * into here. Appears to always returns the largest 32-bit integer value
 * (0xFFFFFFFF)
 * @return true If reading for breakpoints was a success.
 * @return false If reading for breakpoints was a failure.
 */
const bool getBreakpointStatus(unsigned int* wordA, unsigned int* wordB) {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::BP_GET));
  return not((boardGetNBytes((int*)wordA, 4) != 4) ||
             (boardGetNBytes((int*)wordB, 4) != 4));
}

/**
 * @brief Reads a breakpoint definition for use by the breakpoints callbacks.
 * @param breakpointlistIndex Internally within Jimulator, breakpoints are
 * stored as an unordered list. This value the index within that list of
 * breakpoints which should be read. `breakpointlistIndex` has a maximum value
 * of 32, as there can only be 32 breakpoints.
 * @param bp The breakpoint information read from Jimulator will be
 * stored in this struct.
 * @return bool true if reading was a success, else false.
 */
const bool getBreakpointDefinition(unsigned int breakpointlistIndex,
                                   BreakpointInfo* bp) {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::BP_READ));
  boardSendChar(breakpointlistIndex);  // send the number of the definition

  if ((2 != boardGetNBytes((int*)&(bp->misc), 2)) ||
      (4 != boardGetCharArray(4, bp->addressA)) ||
      (4 != boardGetCharArray(4, bp->addressB)) ||
      (8 != boardGetCharArray(8, bp->dataA)) ||
      (8 != boardGetCharArray(8, bp->dataB))) {
    return false;
  } else {
    return true;
  }
}

/**
 * @brief Takes two string literals of equal length which represent string forms
 * of integers - for example, the number 5 represented as the string {'5'}, or
 * the number 327 represented as the string {'3', '2', '7'} - and performs
 * subtraction on the integer values they represent as strings, returning that
 * value as an int.
 * For example, s1 = "400", s2 = "350", the return value 50.
 * @param s1 The "number" to have its value subtracted from.
 * @param s2 The "number" to subtract by.
 * @return int The output of the arithmetic.
 */
constexpr const int numericStringSubtraction(const unsigned char* const s1,
                                             const unsigned char* const s2) {
  int ret = 0;

  for (int i = ADDRESS_BUS_WIDTH; i--;) {
    ret = rotateLeft1Byte(ret) + (int)s1[i] - (int)s2[i];
  }
  return ret;
}

/**
 * @brief Copy one string literal into another string literal.
 * @param i The length of the string literals.
 * @param source The value to be copied.
 * @param dest The location to copy the value into.
 */
constexpr void copyStringLiterals(int i,
                                  unsigned char* source,
                                  unsigned char* dest) {
  while (i--) {
    dest[i] = source[i];
  }
}

/**
 * @brief Overwrites some breakpoint information into Jimulator.
 * @param wordA The value to set the breakpoint to. POSSIBLY represents whether
 * the breakpoint is active? (e.g. 0 for inactive)
 * @param wordB The index that the breakpoint exists within the list.
 */
void setBreakpointStatus(unsigned int wordA, unsigned int wordB) {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::BP_SET));
  boardSendNBytes(wordA, 4);  // send word a
  boardSendNBytes(wordB, 4);  // send word b
}

/**
 * @brief Writes a new breakpoint into the breakpoint list.
 * @param breakpointIndex The index that this breakpoint will exist in within
 * Jimulators internal list of breakpoint.
 * @param bp A struct containing all of the information about the new
 * breakpoint.
 */
void setBreakpointDefinition(unsigned int breakpointIndex, BreakpointInfo* bp) {
  boardSendChar(static_cast<unsigned char>(BoardInstruction::BP_WRITE));
  boardSendChar(breakpointIndex);  // send the list index of the breakpoint
  boardSendNBytes(bp->misc, 2);
  boardSendCharArray(ADDRESS_BUS_WIDTH, bp->addressA);
  boardSendCharArray(ADDRESS_BUS_WIDTH, bp->addressB);
  boardSendCharArray(8, bp->dataA);
  boardSendCharArray(8, bp->dataB);
}

/**
 * @brief Sends an array of characters to Jimulator.
 * @param length The number of bytes to send from data.
 * @param data An pointer to the data that should be sent.
 */
void boardSendCharArray(int length, unsigned char* data) {
  struct pollfd pollfd;
  pollfd.fd = writeToJimulator;
  pollfd.events = POLLOUT;

  // See if output possible
  if (not poll(&pollfd, 1, OUT_POLL_TIMEOUT)) {
    std::cout << "Client system not responding!\n";  // communication problem
  }

  // Write char_number bytes
  if (write(writeToJimulator, data, length) == -1) {
    std::cout << "Pipe write error!\n";
  }
}

/**
 * @brief Sends a singular character to Jimulator.
 * @param data The character to send.
 */
void boardSendChar(unsigned char to_send) {
  boardSendCharArray(1, &to_send);
}

/**
 * @brief reads an array of characters from Jimulator.
 * @param length The number of characters to read from Jimulator.
 * @param data A pointer to where to store the data rea from Jimulator.
 * @return int The number of bytes successfully received, up to `length` number
 * of characters.
 */
const int boardGetCharArray(int char_number, unsigned char* data_ptr) {
  int reply_count;  // Number of chars fetched in latest attempt
  int reply_total = 0;
  struct pollfd pollfd;

  pollfd.fd = readFromJimulator;
  pollfd.events = POLLIN;

  // while there is more to get
  while (char_number > 0) {
    // If nothing available
    if (not poll(&pollfd, 1, IN_POLL_TIMEOUT)) {
      reply_count = 0;  // Will force loop termination
    }

    // attempt to read the number of bytes requested  and store the number of
    // bytes received
    else {
      reply_count = read(readFromJimulator, data_ptr, char_number);
    }

    if (reply_count == 0) {
      char_number = -1;  // Set to terminate
    }

    // Set minimum to 0
    if (reply_count < 0) {
      reply_count = 0;
    }

    reply_total += reply_count;
    char_number -= reply_count;  // Update No. bytes that are still required
    data_ptr += reply_count;     // Move the data pointer to its new location
  }

  return reply_total;  // return the number of bytes received
}

/**
 * @brief Reads a singular character from Jimulator.
 * @param data A pointer to a memory location where the read data can be stored.
 * @return int The number of bytes successfully received - either 1 or 0.
 */
const int boardGetChar(unsigned char* to_get) {
  return boardGetCharArray(1, to_get);
}

/**
 * @brief Reads n bytes of data from Jimulator.
 * @warning This function reads data in a little-endian manner - that is, the
 * least significant bit of data is on the left side of the array. Most systems
 * use a big-endian architecture, so this may require conversion.
 * @param data A pointer to a location where the data can be stored.
 * @param n The number of bytes to read.
 * @return int The number of bytes received successfully.
 */
const int boardGetNBytes(int* data, int n) {
  if (n > MAX_SERIAL_WORD) {
    n = MAX_SERIAL_WORD;  // Clip, just in case
  }

  char unsigned buffer[MAX_SERIAL_WORD];
  int numberOfReceivedBytes = boardGetCharArray(n, buffer);
  *data = 0;

  for (int i = 0; i < numberOfReceivedBytes; i++) {
    *data = *data | (getLeastSignificantByte(buffer[i]) << (i * 8));
  }

  return numberOfReceivedBytes;
}

/**
 * @brief Gets a code that indicates the internal state of Jimulator.
 * @return int A code indicating the internal state of Jimulator.
 */
const ClientState getBoardStatus() {
  unsigned char clientStatus = 0;
  int stepsSinceReset;
  int leftOfWalk;

  // If the board sends back the wrong the amount of data
  boardSendChar(static_cast<unsigned char>(BoardInstruction::WOT_U_DO));

  if (boardGetChar(&clientStatus) != 1 ||
      boardGetNBytes(&leftOfWalk, 4) != 4 ||       // Steps remaining
      boardGetNBytes(&stepsSinceReset, 4) != 4) {  // Steps since reset
    std::cout << "board not responding\n";
    return ClientState::BROKEN;
  }

  // TODO: clientStatus represents what the board is doing and why - can be
  //       reflected in the view? and the same with stepsSinceReset

  return static_cast<ClientState>(clientStatus);
}

/**
 * @brief Writes n bytes of data to Jimulator.
 * @warning Jimulator reads data in a little-endian manner - that is, the
 * least significant bit of data is on the left side of a number. Data should be
 * converted into little-endian before being sent.
 * @param data The data to be written.
 * @param n The number of bytes to write.
 */
void boardSendNBytes(int value, int n) {
  unsigned char buffer[MAX_SERIAL_WORD];

  if (n > MAX_SERIAL_WORD) {
    n = MAX_SERIAL_WORD;  // Clip, just in case...
  }

  for (int i = 0; i < n; i++) {
    buffer[i] = getLeastSignificantByte(value);  // Byte into buffer
    value = rotateRight1Byte(value);             // Get next byte
  }

  boardSendCharArray(n, buffer);
}

/**
 * @brief Gets serialized bit data from the board that represents 16 register
 * values - 15 general purpose registers and the PC.
 * @returns const std::array<unsigned char, 64> An array of bytes fetched from
 * Jimulator representing the memory values.
 */
const std::array<unsigned char, 64> readRegistersIntoArray() {
  unsigned char data[64];

  boardSendChar(static_cast<unsigned char>(BoardInstruction::GET_REG));
  boardSendNBytes(0, 4);
  boardSendNBytes(16, 2);
  boardGetCharArray(64, data);

  std::array<unsigned char, 64> ret;
  std::copy(std::begin(data), std::end(data), std::begin(ret));
  return ret;
}

/**
 * @brief Converts an array of integers into a formatted hexadecimal string.
 * @warning Jimulator often treats arrays of characters as plain arrays of bits
 * to be manipulated. In this instance, although it is an array of characters,
 * the paramter `v` is actually an array of integers.
 * @param i The number of bits to read.
 * @param v A pointer to the array of integers.
 * @param prepend0x Whether to prepend the output string with an "0x" or not.
 * @return std::string A hexadecimal formatted register value.
 */
const std::string integerArrayToHexString(int i,
                                          unsigned char* const v,
                                          const bool prepend0x) {
  std::stringstream ss;

  if (prepend0x) {
    ss << "0x";
  }

  while (i--) {
    ss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
       << (int)v[i];
  }

  return ss.str();
}

/**
 * @brief Takes a string literals which represent a string form of an integer -
 * for example, the number 5 represented as the string {'5'}, or the number 327
 * represented as the string {'3', '2', '7'} - and returns the value represented
 * as an int. For example:
 * Input string {'8', '1', '8'} will result in the output 818.
 * @param i The length of the string.
 * @param s The string representation of the number to have its value read.
 * @return int The value read from the string.
 */
constexpr const int numericStringToInt(int i, const unsigned char* const s) {
  int ret = 0;

  while (i--) {
    ret = rotateLeft1Byte(ret) + (s[i]);
  }

  return ret;
}

/**
 * @brief Takes a string representation of an integer - for example, the string
 * {'5' '7' '2'} representing the integer 572 - and an integer number, adding
 * these two "numbers" together into a second string literal. For example:
 * The string {'6' '2'} and the integer 6 will output the string {'6' '6'}. The
 * output of the addition is stored in the string paramter.
 * @param s The string representation of a number, and also where the output of
 * the addition is stored.
 * @param n The integer to add.
 */
constexpr void numericStringAndIntAddition(unsigned char* const s, int n) {
  int temp = 0;

  for (int i = 0; i < ADDRESS_BUS_WIDTH; i++) {
    temp = s[i] + getLeastSignificantByte(n);
    s[i] = getLeastSignificantByte(temp);
    n = rotateRight1Byte(n);  // Shift to next byte
    if (temp >= 0x100) {
      n++;  // Propagate carry
    }
  }
}

/**
 * @brief Calculates the difference between the current address to display and
 * the next address that needs to be displayed.
 * @param src The current line in the source file.
 * @param addr The current address pointed to.
 * @return const int The difference between the current address to display and
 * the next address that needs to be displayed.
 */
constexpr const int disassembleSourceFile(SourceFileLine* src,
                                          unsigned int addr) {
  if (src == NULL || src == nullptr) {
    return 4;
  }

  unsigned int diff = src->address - addr;  // How far to next line start?

  // Do have a source line, but shan't use it
  if (diff == 0) {
    src = src->next;  // Use the one after
    if (src != NULL) {
      diff = src->address - addr;  // if present
    } else {
      diff = 1000;  // Effectively infinity
    }
  }

  if (diff < 4) {
    return diff;  // Next source entry
  } else {
    return 4 - (addr % 4);  // To next word alignment
  }
}

/**
 * @brief Reads all of the breakpoints from Jimulator into a map that can be
 * indexed by address.
 * The address is the key, with a boolean as the value to form a pair. However
 * the boolean is redundant - since the address is the key, if you lookup an
 * address in the map and it is present, you know that a breakpoint was found
 * there.
 * @return const std::unordered_map<u_int32_t, bool> A map of addresses.
 */
const std::unordered_map<u_int32_t, bool> getAllBreakpoints() {
  std::unordered_map<u_int32_t, bool> breakpointAddresses;
  unsigned int wordA, wordB;
  bool error = false;

  // If reading the breakpoints was a success, loops through all of the possible
  // breakpoints - if they are active, add them to the map.
  if (getBreakpointStatus(&wordA, &wordB)) {
    for (int i = 0; (i < MAX_NUMBER_OF_BREAKPOINTS) && not error; i++) {
      if (((wordA >> i) & 1) != 0) {
        BreakpointInfo bp;

        if (getBreakpointDefinition(i, &bp)) {
          u_int32_t addr = numericStringToInt(4, bp.addressA);
          breakpointAddresses.insert({addr, true});
        } else {
          error = true;  // Read failure causes loop termination
        }
      }
    }
  }

  return breakpointAddresses;
}

// ! COMPILING STUFF BELOW! !
// ! COMPILING STUFF BELOW! !
// ! COMPILING STUFF BELOW! !

/**
 * @brief removes all of the old references to the previous file.
 */
void flushSourceFile() {
  SourceFileLine* old = source.pStart;
  source.pStart = NULL;
  source.pEnd = NULL;

  while (old != NULL) {
    if (old->text != NULL) {
      g_free(old->text);
    }

    SourceFileLine* trash = old;
    old = old->next;
    g_free(trash);
  }
}

/**
 * @brief Check if the passed character represents a legal hex digit.
 * @param character the character under test
 * @return int the hex value or -1 if not a legal hex digit
 */
constexpr const int checkHexCharacter(const char character) {
  if ((character >= '0') && (character <= '9')) {
    return character - '0';
  } else if ((character >= 'A') && (character <= 'F')) {
    return character - 'A' + 10;
  } else if ((character >= 'a') && (character <= 'f')) {
    return character - 'a' + 10;
  } else {
    return -1;
  }
}

/**
 * @brief Reads a n from a string of text in the .kmd file.
 * @param f A file handle.
 * @param c A pointer to the current character being read in the file.
 * @param n A pointer for where to read the found n into.
 * @return int The read n.
 */
constexpr const int readNumberFromFile(FILE* const f,
                                       char* const c,
                                       unsigned int* const n) {
  while ((*c == ' ') || (*c == '\t')) {
    *c = getc(f);  // Skip spaces
  }

  int j = 0, value = 0;
  for (int digit = checkHexCharacter(*c); digit >= 0;
       digit = checkHexCharacter(*c), j++) {
    value = (value << 4) | digit;  // Accumulate digit
    *c = getc(f);                  // Get next character
  }  // Exits at first non-hex character (which is discarded)

  j = (j + 1) / 2;  // Round digit count to bytes

  int k = 0;
  if (j == 0) {
    k = 0;
  } else {
    *n = value;  // Only if n found

    if (j > 4) {
      k = 4;  // Currently clips at 32-bit
    } else {
      for (k = 1; k < j; k = k << 1) {
        ;  // Round j to 2^N
      }
    }
  }

  return k;
}

/**
 * @brief Converts a provided memory size into a Jimulator legal memory size.
 * @param size The size to convert.
 * @return unsigned int The Jimulator legal size.
 */
constexpr const unsigned int boardTranslateMemsize(const int size) {
  switch (size) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return 0;
  }
}

/**
 * @brief Sets a memory value of a given address to a new value.
 * This code is LEGACY. It used to run with a check on `board_version`:
 * however `board_version` always passed the check due to the certainty of the
 * "hardware" run under the emulator; therefore the check has been removed.
 * @param address pointer to the address (in bytes)
 * @param value pointer to the new value to be stored
 * @param size width of current memory (in bytes)
 */
void boardSetMemory(unsigned char* const address,
                    unsigned char* const value,
                    const int size) {
  boardSendChar(BoardInstruction::SET_MEM | boardTranslateMemsize(size));
  boardSendCharArray(ADDRESS_BUS_WIDTH, address);  // send address
  boardSendNBytes(1, 2);                           // send width
  boardSendCharArray(size, value);
}

/**
 * @brief Reads the source of the file pointer to by pathToKMD
 * @param pathToKMD A path to the `.kmd` file to be loaded.
 * @return true if successful, false otherwise.
 */
const bool readSourceFile(const char* const pathToKMD) {
  // TODO: this function is a jumbled mess, refactor and remove sections
  unsigned int oldAddress, dSize[SOURCE_FIELD_COUNT],
      dValue[SOURCE_FIELD_COUNT];
  int byteTotal, textLength;
  char buffer[SOURCE_TEXT_LENGTH + 1];  // + 1 for terminator
  SourceFileLine* currentLine;

  // `system` runs the paramter string as a shell command (i.e. it launches a
  // new process) `pidof` checks to see if a process by the name `jimulator` is
  // running. If it fails (non-zero) It will print an error and return failure.
  if (system("pidof -x jimulator > /dev/null")) {
    // TODO: Jimulator is not running... so relaunch Jimulator?
    std::cout << "Jimulator is not running!\n";
    return false;
  }

  // If file cannot be read, return false
  FILE* komodoSource = fopen(pathToKMD, "r");
  if (komodoSource == NULL) {
    std::cout << "Source could not be opened!\n";
    return false;
  }

  bool hasOldAddress = false;  // Don't know where we start

  // Repeat until end of file
  while (not feof(komodoSource)) {
    unsigned int address = 0;     // Really needed?
    bool flag = false;            // Haven't found an address yet
    char c = getc(komodoSource);  // The current character being parsed

    // If the first character is a colon, read a symbol record
    if (c == ':') {
      hasOldAddress = false;  // Don't retain position
    }

    // Read a source line record
    else {
      for (int j = 0; j < SOURCE_FIELD_COUNT; j++) {
        dSize[j] = 0;
        dValue[j] = 0;
      }

      byteTotal = 0;
      flag = readNumberFromFile(komodoSource, &c, &address) != 0;

      // Read a new address - and if we got an address, try for data fields
      if (flag) {
        if (c == ':') {
          c = getc(komodoSource);  // Skip colon
        }

        // Loop on data fields
        // repeat several times or until `illegal' character met
        for (int j = 0; j < SOURCE_FIELD_COUNT; j++) {
          dSize[j] = readNumberFromFile(komodoSource, &c, &dValue[j]);

          if (dSize[j] == 0) {
            break;  // Quit if nothing found
          }

          byteTotal = byteTotal + dSize[j];  // Total input
        }

        oldAddress = address + byteTotal;  // Predicted -next- address
        hasOldAddress = true;
      }
      // Address field not found  Maybe something useable?
      else if (hasOldAddress) {
        address = oldAddress;  // Use predicted address
        flag = true;           // Note we do have an address
      }

      // We have a record with an address
      if (flag) {
        while ((c != ';') && (c != '\n') && not feof(komodoSource)) {
          c = getc(komodoSource);
        }

        // Check for field separator
        if (c == ';') {
          c = getc(komodoSource);
          if (c == ' ') {
            c = getc(komodoSource);  // Skip formatting space
          }

          textLength = 0;  // Measure (& buffer) source line

          // Everything to end of line (or clip)
          while ((c != '\n') && not feof(komodoSource) &&
                 (textLength < SOURCE_TEXT_LENGTH)) {
            buffer[textLength++] = c;
            c = getc(komodoSource);
          }

          buffer[textLength++] = '\0';  // textLength now length incl. '\0'
          currentLine = g_new(SourceFileLine, 1);  // Create new record
          currentLine->address = address;

          byteTotal = 0;  // Inefficient
          for (int j = 0; j < SOURCE_FIELD_COUNT; j++) {
            currentLine->dataSize[j] = dSize[j];  // Bytes, not digits
            currentLine->dataValue[j] = dValue[j];

            // clips memory load - debatable
            if ((currentLine->dataSize[j] > 0) &&
                ((currentLine->dataSize[j] + byteTotal) <= SOURCE_BYTE_COUNT)) {
              unsigned char addr[4], data[4];

              for (int i = 0; i < 4; i++) {
                addr[i] =
                    getLeastSignificantByte((address + byteTotal) >> (8 * i));
              }
              for (int i = 0; i < currentLine->dataSize[j]; i++) {
                data[i] = getLeastSignificantByte(currentLine->dataValue[j] >>
                                                  (8 * i));
              }

              // Ignore Boolean error return value for now
              boardSetMemory(addr, data, currentLine->dataSize[j]);
            }

            byteTotal = byteTotal + currentLine->dataSize[j];
            currentLine->hasData = (byteTotal != 0);  // If blank line
          }

          // clips source record - essential
          if (byteTotal > SOURCE_BYTE_COUNT) {
            int m = 0;

            for (int j = 0; j < SOURCE_FIELD_COUNT; j++) {
              m = m + currentLine->dataSize[j];
              if (m <= SOURCE_BYTE_COUNT) {
                dSize[j] = 0;
                byteTotal = byteTotal - currentLine->dataSize[j];
              } else {
                dSize[j] = currentLine->dataSize[j];  // Bytes, not digits
                currentLine->dataSize[j] = 0;
              }
            }

            // error here?
            std::cout << "OVERFLOW " << dSize[0] << " " << dSize[1] << " "
                      << dSize[2] << " " << dSize[3] << " " << byteTotal
                      << std::endl;
          }

          // Copy text to buffer
          currentLine->text = g_new(char, textLength);
          for (int j = 0; j < textLength; j++) {
            currentLine->text[j] = buffer[j];
          }

          SourceFileLine* temp1 = source.pStart;
          SourceFileLine* temp2 = NULL;

          while ((temp1 != NULL) && (address >= temp1->address)) {
            temp2 = temp1;
            temp1 = temp1->next;
          }

          currentLine->next = temp1;
          currentLine->prev = temp2;

          if (temp1 != NULL) {
            temp1->prev = currentLine;
          } else {
            source.pEnd = currentLine;
          }

          if (temp2 != NULL) {
            temp2->next = currentLine;
          } else {
            source.pStart = currentLine;
          }
        }
      }  // Source line
    }

    // Get the next relevant character
    while ((c != '\n') && not feof(komodoSource)) {
      c = getc(komodoSource);
    }
  }

  fclose(komodoSource);
  return true;
}
