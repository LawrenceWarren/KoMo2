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
const clientState getBoardStatus();
const std::array<unsigned char, 64> readRegistersIntoArray();
const int disassembleSourceFile(sourceFileLine*, unsigned int, int);

// Low level sending
void boardSendNBytes(int, int);
void boardSendChar(unsigned char);
void boardSendCharArray(int, unsigned char*);

// Low level receiving
const int boardGetNBytes(int*, int);
const int boardGetChar(unsigned char*);
const int boardGetCharArray(int, unsigned char*);

// Breakpoints
const bool getBreakpointStatus(unsigned int*, unsigned int*);
const bool getBreakpointDefinition(unsigned int, breakpointInfo*);
void setBreakpointStatus(unsigned int, unsigned int);
void setBreakpointDefinition(unsigned int, breakpointInfo*);
const std::unordered_map<u_int32_t, bool> getAllBreakpoints();

// Helpers
constexpr void copyStringLiterals(int, unsigned char*, unsigned char*);
constexpr const int numericStringSubtraction(int,
                                             unsigned char*,
                                             unsigned char*);
constexpr const int numericStringToInt(int, unsigned char* const);
constexpr void numericStringAndIntAdition(const int,
                                          unsigned char* const,
                                          int,
                                          unsigned char* const);
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
  if (checkBoardState() == clientState::NORMAL ||
      Jimulator::checkBoardState() == clientState::BREAKPOINT) {
    boardSendChar(static_cast<unsigned char>(boardInstruction::START));
    boardSendNBytes(steps, 4);  // Send step count
  }
}

/**
 * @brief Continues running Jimulator.
 */
void Jimulator::continueJimulator() {
  if (Jimulator::checkBoardState() == clientState::NORMAL ||
      Jimulator::checkBoardState() == clientState::BREAKPOINT) {
    boardSendChar(static_cast<unsigned char>(boardInstruction::CONTINUE));
  }
}

/**
 * @brief Pauses the emulator running.
 */
void Jimulator::pauseJimulator() {
  boardSendChar(static_cast<unsigned char>(boardInstruction::STOP));
}

/**
 * @brief Reset the emulators running.
 */
void Jimulator::resetJimulator() {
  boardSendChar(static_cast<unsigned char>(boardInstruction::RESET));
}

/**
 * @brief Sets a breakpoint.
 * @param addr The address to set the breakpoint at.
 * @return const bool If setting the breakpoint succeeded.
 */
const bool Jimulator::setBreakpoint(uint32_t addr) {
  unsigned int wordA, wordB;
  unsigned char address[ADDRESS_BUS_WIDTH];

  // Unpack address to byte array
  for (int i = 0; i < ADDRESS_BUS_WIDTH; i++) {
    address[i] = getLeastSignificantByte(addr >> (8 * i));
  }

  if (not getBreakpointStatus(&wordA, &wordB)) {
    // TODO: handle this event gracefully?
    return false;
  }

  bool breakpoint_found = false;

  // Maximum of 32 breakpoints - loop through to see if more can be set
  for (int i = 0; i < MAX_NUMBER_OF_BREAKPOINTS; i++) {
    if (((wordA >> i) & 1) != 0) {
      breakpointInfo bp;  // Space to store fetched definition

      bool error = not getBreakpointDefinition(i, &bp);

      // If this breakpoint was found
      if (not error && (not numericStringSubtraction(ADDRESS_BUS_WIDTH, address,
                                                     bp.addressA))) {
        breakpoint_found = true;
        setBreakpointStatus(0, 1 << i);
      }
    }
  }

  // breakpoint(s) matched and deleted ???
  if (breakpoint_found) {
    return false;
  }
  // See if we can set breakpoint
  else {
    int temp = (~wordA) & wordB;  // Undefined => possible choice

    // If any free entries ...
    if (temp != 0) {  // Define (set) breakpoint
      breakpointInfo bp;
      int count, i;

      for (i = 0; (((temp >> i) & 1) == 0); i++)
        ;

      // Choose free(?) number
      bp.misc = -1;  // Really two byte parameters

      // Should send 2*words address then two*double words data @@@
      copyStringLiterals(ADDRESS_BUS_WIDTH, address, bp.addressA);
      for (count = 0; count < ADDRESS_BUS_WIDTH; count++) {
        bp.addressB[count] = 0xFF;
      }
      for (count = 0; count < 8; count++) {
        bp.dataA[count] = 0x00;
        bp.dataB[count] = 0x00;
      }

      setBreakpointDefinition(i, &bp);
      return true;
    } else {
      return false;
    }
  }
}

/**
 * @brief Check the state of the board - logs what it is doing.
 * @return int 0 if the board is in a failed state, else a number greater than
 * 0.
 */
const clientState Jimulator::checkBoardState() {
  const auto board_state = getBoardStatus();

  // Check and log error states
  switch (board_state) {
    case clientState::RUNNING_SWI:
      break;
    case clientState::RUNNING:
      break;
    case clientState::STEPPING:
      break;
    case clientState::MEMFAULT:
      break;
    case clientState::BUSY:
      break;
    case clientState::FINISHED:
      break;
    case clientState::BREAKPOINT:
      break;
    default:
      return clientState::NORMAL;
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
  unsigned char
      string[256];  // (large enough) string to get the message from the board
  unsigned char length;

  std::string output("");

  do {
    boardSendChar(static_cast<unsigned char>(boardInstruction::FR_READ));
    boardSendChar(0);       // send the terminal number
    boardSendChar(32);      // send the maximum length possible
    boardGetChar(&length);  // get length of message

    // non-zero received from board - not an empty packet
    if (length != 0) {
      boardGetCharArray(length,
                        string);  // get the message recorded in string
      string[length] = '\0';
      output.append((char*)string);
    }
  } while (length == 255);

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
        boardInstruction::FR_WRITE));  // begins a write
    boardSendChar(0);                  // tells where to send it
    boardSendChar(1);                  // send length 1
    boardSendChar(key_pressed);        // send the message - 1 char currently
    boardGetChar(&res);                // Read the result
    return true;
  }

  return false;
}

/**
 * @brief Get the memory values from Jimulator, starting to s_address_int.
 * @param s_address_int The address to start at, as an integer.
 * @return std::array<Jimulator::MemoryValues, 13> An array of all of the
 * values read from Jimulator, including each column.
 */
std::array<Jimulator::MemoryValues, 13> Jimulator::getJimulatorMemoryValues(
    const uint32_t s_address_int) {
  const int count = 13;  // The number of values displayed in the memory window
  const int bytecount =
      count * ADDRESS_BUS_WIDTH;  // The number of bytes to fetch from memory

  // Bit level hacking happening here - converting the integer address into
  // an array of characters.
  unsigned char* p = (unsigned char*)&s_address_int;
  unsigned char start_address[ADDRESS_BUS_WIDTH] = {p[0], p[1], p[2], p[3]};
  start_address[0] &= -4;  // Normalise down to the previous multiple of 4

  unsigned char memdata[bytecount];

  // Reading data into arrays!
  boardSendChar(static_cast<unsigned char>(boardInstruction::GET_MEM));
  boardSendCharArray(ADDRESS_BUS_WIDTH, start_address);
  boardSendNBytes(count, 2);
  boardGetCharArray(bytecount, memdata);

  // ! Dangerous old logic ahead is used to read memory.
  // ! This is very much C-style code, be careful

  // Switches the endianness of the address
  unsigned char address[ADDRESS_BUS_WIDTH] = {
      start_address[0], start_address[1], start_address[2], start_address[3]};

  sourceFileLine* src = NULL;
  bool used_first = false;
  unsigned int start_addr =
      numericStringToInt(ADDRESS_BUS_WIDTH, start_address);

  // Setup src variable
  if (source.pStart != NULL) {
    src = source.pStart;  // Known to be valid
    while ((src != NULL) && ((src->address < start_addr) || src->nodata)) {
      src = src->pNext;
    }

    // We fell off the end; wrap to start
    if (src == NULL) {
      src = source.pStart;
      used_first = true;
      // Find a record with some data
      while ((src != NULL) && src->nodata) {
        src = src->pNext;
      }
    }
  }

  // Data is read into this array
  std::array<Jimulator::MemoryValues, 13> readValues;
  const auto bps = getAllBreakpoints();

  // Iterate over display rows
  for (long unsigned int row = 0; row < readValues.size(); row++) {
    int increment = 4;

    unsigned int addr = numericStringToInt(ADDRESS_BUS_WIDTH, address);
    readValues[row].address = addr;

    // If the source file can be read
    if (src != NULL) {
      // If the src address is valid
      if (addr == src->address) {
        increment = 0;              // How far should we move?
        char* pStr = g_strdup("");  // Seed hex string

        // Source field entries
        for (int i = 0; i < SOURCE_FIELD_COUNT; i++) {
          if (src->data_size[i] > 0) {
            char spaces[5];  // Max. one per byte plus terminator

            auto data =
                integerArrayToHexString(src->data_size[i],
                                        &memdata[addr - start_addr + increment])
                    .c_str();

            int j = 0;
            for (; j < src->data_size[i]; j++) {
              spaces[j] = ' ';
            }

            spaces[j] = '\0';
            auto trash = pStr;
            pStr = g_strconcat(pStr, data, spaces, NULL);
            g_free(trash);
          }
          increment = increment + src->data_size[i];
        }

        readValues[row].hex = std::string(pStr);

        // Remove any comments that trail the disassembly
        readValues[row].disassembly =
            std::regex_replace(std::string(src->text), std::regex(";.*$"), "");

        g_free(pStr);

        do {
          if (src->pNext != NULL) {
            src = src->pNext;  // Move on ...
          } else {             // ... wrapping, if required
            if (not used_first) {
              src = source.pStart;
              used_first = true;
            } else {
              src = NULL; /* Been here before - give in */
            }
          }
        } while ((src != NULL) && src->nodata);
      } else {
        // If a src line is invalid
        readValues[row].hex = std::string("00000000");
        readValues[row].disassembly = std::string("...");
        increment = disassembleSourceFile(src, addr, increment);
      }
    } else {
      // If there is no src line to read
      readValues[row].hex = std::string("00000000");
      readValues[row].disassembly = std::string("...");
    }

    // Find if a breakpoint is set on this line
    auto iter = bps.find(readValues[row].address);

    // if the iterator is not at the end of the map, the breakpoint was found
    // and can be set
    if (iter != bps.end()) {
      readValues[row].breakpoint = true;
    }

    // Move the address on
    numericStringAndIntAdition(ADDRESS_BUS_WIDTH, address, increment, address);
  }

  return readValues;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!! Functions below are not included in the header file !!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

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
 * (0xFFFFFFFF).
 * @return true If reading for breakpoints was a success.
 * @return false If reading for breakpoints was a failure.
 */
const bool getBreakpointStatus(unsigned int* wordA, unsigned int* wordB) {
  boardSendChar(static_cast<unsigned char>(boardInstruction::BP_GET));
  return not((boardGetNBytes((int*)wordA, 4) != 4) ||
             (boardGetNBytes((int*)wordB, 4) != 4));
}

/**
 * @brief Reads a breakpoint definition for use by the breakpoints callbacks.
 * @param breakpointlistIndex Internally within Jimulator, breakpoints are
 * stored as an unordered list. This value the index within that list of
 * breakpoints which should be read. `breakpointlistIndex` has a maximum value
 * of 32, as there can only be 32 breakpoints.
 * @param breakpointInfo The breakpoint information read from Jimulator will be
 * stored in this struct.
 * @return bool true if reading was a success.
 * @return bool false If reading was a failure.
 */
const bool getBreakpointDefinition(unsigned int trap_number,
                                   breakpointInfo* breakpointInfo) {
  boardSendChar(static_cast<unsigned char>(boardInstruction::BP_READ));
  boardSendChar(trap_number);  // send the number of the definition

  if ((2 != boardGetNBytes((int*)&((*breakpointInfo).misc),
                           2))  // get the trap misc properties
      || (4 != boardGetCharArray(4, (*breakpointInfo).addressA)) ||
      (4 != boardGetCharArray(4, (*breakpointInfo).addressB)) ||
      (8 != boardGetCharArray(8, (*breakpointInfo).dataA)) ||
      (8 != boardGetCharArray(8, (*breakpointInfo).dataB))) {
    // get address a&b and data a&b
    return false;
  } else
    return true;
}

/**
 * @brief Takes two string literals of equal length which represent string forms
 * of integers - for example, the number 5 represented as the string {'5'}, or
 * the number 327 represented as the string {'3', '2', '7'} - and performs
 * subtraction on the integer values they represent as strings, returning that
 * value as an int.
 * For example, s1 = "400", s2 = "350", the return value 50.
 * @param i The length of the two strings.
 * @param s1 The "number" to have its value subtracted from.
 * @param s2 The "number" to subtract by.
 * @return int The output of the arithmetic.
 */
constexpr const int numericStringSubtraction(int i,
                                             unsigned char* s1,
                                             unsigned char* s2) {
  int ret = 0; /* bit array - bit array => int */

  while (i--) {
    ret = (ret << 8) + (int)s1[i] - (int)s2[i];
  }
  return ret;  // Carry propagated because intermediate -int- can be negative
}

/**
 * @brief Overwrites some breakpoint information into Jimulator.
 * @param wordA The value to set the breakpoint to. POSSIBLY represents whether
 * the breakpoint is active? (e.g. 0 for inactive)
 * @param wordB The index that the breakpoint exists within the list.
 */
void setBreakpointStatus(unsigned int wordA, unsigned int wordB) {
  boardSendChar(static_cast<unsigned char>(boardInstruction::BP_SET));
  boardSendNBytes(wordA, 4);  // send word a
  boardSendNBytes(wordB, 4);  // send word b
  return;
}

/**
 * @brief Copy one string literal into another string literal.
 * @param count The length of the string literals.
 * @param source The value to be copied.
 * @param destination The location to copy the value into.
 */
constexpr void copyStringLiterals(int count,
                                  unsigned char* source,
                                  unsigned char* destination) {
  while (count--) {
    destination[count] = source[count];  // Copy char array
  }
}

/**
 * @brief Writes a new breakpoint into the breakpoint list.
 * @param breakpointIndex The index that this breakpoint will exist in within
 * Jimulators internal list of breakpoint.
 * @param newBreakpoint A struct containing all of the information about the new
 * breakpoint.
 */
void setBreakpointDefinition(unsigned int trap_number,
                             breakpointInfo* breakpointInfon) {
  boardSendChar(static_cast<unsigned char>(boardInstruction::BP_WRITE));
  boardSendChar(trap_number);  // send the list index of the breakpoint
  boardSendNBytes(((*breakpointInfon).misc), 2);
  boardSendCharArray(ADDRESS_BUS_WIDTH, (*breakpointInfon).addressA);
  boardSendCharArray(ADDRESS_BUS_WIDTH, (*breakpointInfon).addressB);
  boardSendCharArray(8, (*breakpointInfon).dataA);
  boardSendCharArray(8, (*breakpointInfon).dataB);
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
const int boardGetNBytes(int* val_ptr, int n) {
  if (n > MAX_SERIAL_WORD) {
    n = MAX_SERIAL_WORD;  // Clip, just in case
  }

  char unsigned buffer[MAX_SERIAL_WORD];
  int numberOfReceivedBytes = boardGetCharArray(n, buffer);
  *val_ptr = 0;

  for (int i = 0; i < numberOfReceivedBytes; i++) {
    *val_ptr = *val_ptr | (getLeastSignificantByte(buffer[i])
                           << (i * 8));  // Assemble integer
  }

  return numberOfReceivedBytes;
}

/**
 * @brief Gets a code that indicates the internal state of Jimulator.
 * @return int A code indicating the internal state of Jimulator.
 */
const clientState getBoardStatus() {
  unsigned char clientStatus = 0;
  int stepsSinceReset;
  int leftOfWalk;

  // If the board sends back the wrong the amount of data
  boardSendChar(static_cast<unsigned char>(boardInstruction::WOT_U_DO));

  if (boardGetChar(&clientStatus) != 1 ||
      boardGetNBytes(&leftOfWalk, 4) != 4 ||       // Steps remaining
      boardGetNBytes(&stepsSinceReset, 4) != 4) {  // Steps since reset
    std::cout << "board not responding\n";
    return clientState::BROKEN;
  }

  /* TODO: clientStatus represents what the board is doing and why - can be
   reflected in the view? and the same with stepsSinceReset
   */

  return static_cast<clientState>(clientStatus);
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
    value = value >> 8;                          // Get next byte
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

  boardSendChar(static_cast<unsigned char>(boardInstruction::GET_REG));
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
constexpr const int numericStringToInt(int i, unsigned char* const s) {
  int ret = 0;

  while (i--) {
    ret = (ret << 8) + (s[i]);
  }

  return ret;
}

/**
 * @brief Takes a string representation of an integer - for example, the string
 * {'5' '7' '2'} representing the integer 572 - and an integer number, adding
 * these two "numbers" together into a second string literal. For example:
 * The string {'6' '2'} and the integer 6 will output the string {'6' '6'}.
 * @param length The length of the string.
 * @param data The string representation of a number.
 * @param number The integer to add.
 * @param out Where to store the output string.
 */
constexpr void numericStringAndIntAdition(const int length,
                                          unsigned char* const data,
                                          int number,
                                          unsigned char* const out) {
  int temp = 0;

  for (int i = 0; i < length; i++) {
    temp = data[i] + getLeastSignificantByte(number);  // Add next 8 bits
    out[i] = getLeastSignificantByte(temp);
    number = number >> 8;  // Shift to next byte
    if (temp >= 0x100) {
      number++;  // Propagate carry
    }
  }
}

/**
 * @brief Calculates the difference between the current address to display and
 * the next address that needs to be displayed.
 * @param src The current line in the source file.
 * @param addr The current address pointed to.
 * @param increment The amount to increment addresses by.
 * @return int The difference between the current address to display and the
 * next address that needs to be displayed.
 */
const int disassembleSourceFile(sourceFileLine* src,
                                unsigned int addr,
                                int increment) {
  unsigned int diff;

  diff = src->address - addr;  // How far to next line start?

  // Do have a source line, but shan't use it
  if (diff == 0) {
    src = src->pNext;  // Use the one after ...
    if (src != NULL) {
      diff = src->address - addr;  // if present/
    } else {
      diff = 1000;  // Effectively infinity
    }
  }

  if (diff < 4) {
    increment = diff;  // Next source entry
  } else {
    increment = 4 - (addr % 4);  // To next word alignment
  }

  return increment;  // A hack when routine was extracted from below
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

  // Have trap status info okay: get breaks and breaksinactive list
  if (getBreakpointStatus(&wordA, &wordB)) {
    // Loop through every breakpoint
    for (int i = 0; (i < MAX_NUMBER_OF_BREAKPOINTS) && not error; i++) {
      // If breakpoint found, read the breakpoint value
      if (((wordA >> i) & 1) != 0) {
        breakpointInfo trap;

        // if okay
        if (getBreakpointDefinition(i, &trap)) {
          // Get address and save into map
          u_int32_t addr = numericStringToInt(4, trap.addressA);
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
  sourceFileLine *pOld, *pTrash;

  pOld = source.pStart;
  source.pStart = NULL;
  source.pEnd = NULL;

  while (pOld != NULL) {
    if (pOld->text != NULL) {
      g_free(pOld->text);
    }

    pTrash = pOld;
    pOld = pOld->pNext;
    g_free(pTrash);
  }
}

/**
 * @brief Check if the passed character represents a legal hex digit.
 * @param character the character under test
 * @return int the hex value or -1 if not a legal hex digit
 */
const int checkHexCharacter(const char character) {
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
 * @brief Reads a number from a string of text in the .kmd file.
 * @param fHandle A file handle.
 * @param pC A pointer to the current character being read in the file.
 * @param number A pointer for where to read the found number into.
 * @return int The read number.
 */
const int readNumberFromFile(FILE* const fHandle,
                             char* const pC,
                             unsigned int* const number) {
  while ((*pC == ' ') || (*pC == '\t')) {
    *pC = getc(fHandle);  // Skip spaces
  }

  int j = 0, value = 0;
  for (int digit = checkHexCharacter(*pC); digit >= 0;
       digit = checkHexCharacter(*pC), j++) {
    value = (value << 4) | digit;  // Accumulate digit
    *pC = getc(fHandle);           // Get next character
  }  // Exits at first non-hex character (which is discarded)

  j = (j + 1) / 2;  // Round digit count to bytes

  int k;
  if (j == 0) {
    k = 0;
  } else {
    *number = value;  // Only if number found

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
 * @param count number of elements
 * @param address pointer to the address (in bytes)
 * @param value pointer to the new value to be stored
 * @param size width of current memory (in bytes)
 */
void boardSetMemory(const int count,
                    unsigned char* const address,
                    unsigned char* const value,
                    const int size) {
  const int bytecount = count * size;
  boardSendChar(boardInstruction::SET_MEM | boardTranslateMemsize(size));
  boardSendCharArray(4, address);  // send address
  boardSendNBytes(count, 2);       // send width
  boardSendCharArray(bytecount, value);
}

/**
 * @brief Reads the source of the file pointer to by pathToKMD
 * @param pathToKMD A path to the `.kmd` file to be loaded.
 * @return true if successful.
 */
const bool readSourceFile(const char* const pathToKMD) {
  // TODO: this function is a jumbled mess, refactor and remove sections
  unsigned int address, old_address;
  unsigned int d_size[SOURCE_FIELD_COUNT], d_value[SOURCE_FIELD_COUNT];
  int i, j, m;
  bool flag;
  int byte_total, text_length;
  char c, buffer[SOURCE_TEXT_LENGTH + 1];  // + 1 for terminator
  sourceFileLine *pNew, *pTemp1, *pTemp2;
  struct stat status;

  // `system` runs the paramter string as a shell command (i.e. it launches a
  // new process). `pidof` checks to see if a process by the name `jimulator` is
  // running. If it fails (non-zero) It will print an error and return failure.
  if (system("pidof -x jimulator > /dev/null")) {
    // TODO: Jimulator is not running... so relaunch Jimulator?
    std::cout << "Jimulator is not running!\n";
    return false;
  }

  FILE* komodoSource = fopen(pathToKMD, "r");
  if (komodoSource == NULL) {
    std::cout << "Source could not be opened!\n";
    return false;
  }

  bool has_old_addr = false;  // Don't know where we start
  stat(pathToKMD, &status);

  // Repeat until end of file
  while (not feof(komodoSource)) {
    address = 0;   // Really needed?
    flag = false;  // Haven't found an address yet
    c = getc(komodoSource);

    // If the first character is a colon, read a symbol record
    if (c == ':') {
      has_old_addr = false;  // Don't retain position
    }

    // Read a source line record
    else {
      for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
        d_size[j] = 0;
        d_value[j] = 0;
      }

      byte_total = 0;
      flag = (readNumberFromFile(komodoSource, &c, &address) != 0);

      // Read a new address - and if we got an address, try for data fields
      if (flag) {
        if (c == ':') {
          c = getc(komodoSource);  // Skip colon
        }

        // Loop on data fields
        // repeat several times or until `illegal' character met
        for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
          d_size[j] = readNumberFromFile(komodoSource, &c, &d_value[j]);

          if (d_size[j] == 0) {
            break;  // Quit if nothing found
          }

          byte_total = byte_total + d_size[j];  // Total input
        }

        old_address = address + byte_total;  // Predicted -next- address
        has_old_addr = true;
      }
      // Address field not found  Maybe something useable?
      else if (has_old_addr) {
        address = old_address;  // Use predicted address
        flag = true;            // Note we do have an address
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

          text_length = 0;  // Measure (& buffer) source line

          // Everything to end of line (or clip)
          while ((c != '\n') && not feof(komodoSource) &&
                 (text_length < SOURCE_TEXT_LENGTH)) {
            buffer[text_length++] = c;
            c = getc(komodoSource);
          }

          buffer[text_length++] = '\0';     // text_length now length incl. '\0'
          pNew = g_new(sourceFileLine, 1);  // Create new record
          pNew->address = address;
          pNew->corrupt = false;

          byte_total = 0;  // Inefficient
          for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
            pNew->data_size[j] = d_size[j];  // Bytes, not digits
            pNew->data_value[j] = d_value[j];

            // clips memory load - debatable
            if ((pNew->data_size[j] > 0) &&
                ((pNew->data_size[j] + byte_total) <= SOURCE_BYTE_COUNT)) {
              unsigned char addr[4], data[4];

              for (i = 0; i < 4; i++) {
                addr[i] =
                    getLeastSignificantByte((address + byte_total) >> (8 * i));
              }
              for (i = 0; i < pNew->data_size[j]; i++) {
                data[i] =
                    getLeastSignificantByte(pNew->data_value[j] >> (8 * i));
              }

              // Ignore Boolean error return value for now
              boardSetMemory(1, addr, data, pNew->data_size[j]);
            }

            byte_total = byte_total + pNew->data_size[j];
            pNew->nodata = (byte_total == 0); /* Mark record if `blank' line */
          }

          // clips source record - essential
          if (byte_total > SOURCE_BYTE_COUNT) {
            m = 0;

            for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
              m = m + pNew->data_size[j];
              if (m <= SOURCE_BYTE_COUNT) {
                d_size[j] = 0;
                byte_total = byte_total - pNew->data_size[j];
              } else {
                d_size[j] = pNew->data_size[j];  // Bytes, not digits
                pNew->data_size[j] = 0;
              }
            }

            // FIXME
            std::cout << "OVERFLOW " << d_size[0] << " " << d_size[1] << " "
                      << d_size[2] << " " << d_size[3] << " " << byte_total
                      << std::endl;
            // Extend with some more records here? @@@ (plant in memory,
            // above, also)
          }

          // Copy text to buffer
          pNew->text = g_new(char, text_length);
          for (j = 0; j < text_length; j++) {
            pNew->text[j] = buffer[j];
          }

          pTemp1 = source.pStart;  // Place new record in address ordered list
          pTemp2 = NULL;  // behind any earlier records with same address.

          while ((pTemp1 != NULL) && (address >= pTemp1->address)) {
            pTemp2 = pTemp1;
            pTemp1 = pTemp1->pNext;
          }

          pNew->pNext = pTemp1;
          pNew->pPrev = pTemp2;

          if (pTemp1 != NULL) {
            pTemp1->pPrev = pNew;
          } else {
            source.pEnd = pNew;
          }

          if (pTemp2 != NULL) {
            pTemp2->pNext = pNew;
          } else {
            source.pStart = pNew;
          }
        }
      }  // Source line
    }

    while ((c != '\n') && not feof(komodoSource)) {
      c = getc(komodoSource);  // Next line anyway
    }
  }

  fclose(komodoSource);
  return true;
}
