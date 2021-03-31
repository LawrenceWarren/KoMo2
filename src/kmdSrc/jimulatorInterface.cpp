/**
 * @file jimulatorInterface.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Contains functionality relating to the serialization, transmission and
 * reception of data to and from Jimulator, with a minor amount of processing
 * done either way.
 * @version 0.1
 * @date 2020-12-29
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
#include <gdk/gdkkeysyms.h>  // Definitions of `special' keys
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
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "globals.h"

// ! Forward declaring auxiliary load functions
void flush_source();
const int boardSendChar(unsigned char to_send);
const int readSource(const char* pathToKMD);
const int callback_memory_refresh();
void run_board(const int steps);
const int board_enq();
int board_send_n_bytes(int value, int n);
int board_get_n_bytes(int* val_ptr, int n);
int boardGetCharArray(int char_number, unsigned char* data_ptr);
int boardSendCharArray(int char_number, unsigned char* data_ptr);
std::string view_chararr2hexstrbe(int count,
                                  unsigned char* values,
                                  bool prepend = false);
bool trap_get_status(unsigned int trap_type,
                     unsigned int* wordA,
                     unsigned int* wordB);
bool read_trap_defn(unsigned int trap_type,
                    unsigned int trap_number,
                    trap_def* trap_defn);
int misc_chararr_sub_to_int(int count,
                            unsigned char* value1,
                            unsigned char* value2);
void trap_set_status(unsigned int trap_type,
                     unsigned int wordA,
                     unsigned int wordB);
void view_chararrCpychararr(int count,
                            unsigned char* source,
                            unsigned char* destination);
bool write_trap_defn(unsigned int trap_type,
                     unsigned int trap_number,
                     trap_def* trap_defn);
void readRegistersIntoArray(unsigned char* data, unsigned int count);
int boardGetChar(unsigned char* to_get);
const std::unordered_map<u_int32_t, bool> getAllBreakpoints();
int view_chararr2int(int count, unsigned char* array);
int source_disassemble(source_line* src, unsigned int addr, int increment);
void view_chararrAdd(int count,
                     unsigned char* byte_string,
                     int number,
                     unsigned char* acc);

source_file source;

/**
 * @brief Stolen from the original KMD source - runs the file specified by
 * `pathToFile` through the KoMoDo compile script.
 * @param pathToBin An absolute path to the `aasm` binary.
 * @param pathToS An absolute path to the `.s` file to be compiled.
 * @param pathToKMD an absolute path to the `.kmd` file that will be output.
 */
const int Jimulator::compileJimulator(const char* const pathToBin,
                                      const char* const pathToS,
                                      const char* const pathToKMD) {
  close(1);
  dup2(compilerCommunication[1], 1);
  close(2);
  dup2(compilerCommunication[1], 2);

  execlp(pathToBin, "aasm", "-lk", pathToKMD, pathToS, (char*)0);
  std::cout << "Running ARM assembler failed." << std::endl;
  return 1;
}

/**
 * @brief A proven working version of load_data. This is legacy code from
 * KoMoDo, stripped down to disinclude anything related to serial ports,
 * networking, etc - it is pure emulation.
 * @param pathToKMD an absolute path to the `.kmd` file that will be loaded.
 */
const int Jimulator::loadJimulator(const char* const pathToKMD) {
  flush_source();
  return readSource(pathToKMD);
}

/**
 * @brief Commences running the emulator.
 * @param steps The number of steps to run for (0 if indefinite)
 */
void Jimulator::startJimulator(const int steps) {
  run_board(steps);
  std::cout << "RUNNING!🔥" << std::endl;
}

/**
 * @brief Continues running Jimulator.
 */
void Jimulator::continueJimulator() {
  if (not Jimulator::checkBoardState()) {
    boardSendChar(BR_CONTINUE);
    std::cout << "CONTINUED!😜" << std::endl;
  }
}

/**
 * @brief Pauses the emulator running.
 */
void Jimulator::pauseJimulator() {
  boardSendChar(BR_STOP);

  if (not Jimulator::checkBoardState()) {
    std::cout << "Paused!🚀" << std::endl;
  }
}

/**
 * @brief Reset the emulators running.
 */
void Jimulator::resetJimulator() {
  boardSendChar(BR_RESET);
  // board_micro_ping();
  // set_refresh(FALSE, 0); /* Unset refresh button */
  // board_micro_ping();    /* Why TWICE?? @@@ */

  if (not Jimulator::checkBoardState()) {
    return;
  }

  printf("Emulator reset!🤯\n");
}

/**
 * @brief Sets a breakpoint.
 * @param addr The address to set the breakpoint at.
 */
const bool Jimulator::setBreakpoint(uint32_t addr) {
  unsigned int worda, wordb;
  bool error = false;
  unsigned char address[4];

  // Unpack address to byte array
  for (int i = 0; i < 4; i++) {
    address[i] = (addr >> (8 * i)) & 0xFF;
  }

  error = error | trap_get_status(0, &worda, &wordb);

  if (error) {
    // TODO: handle this event gracefully?
    return false;
  }

  int temp;
  int breakpoint_found = FALSE;

  // Maximum of 32 breakpoints - loop through to see if more can be set
  for (int i = 0; i < 32; i++) {
    if (((worda >> i) & 1) != 0) {
      trap_def trap;  // Space to store fetched definition

      error = not read_trap_defn(0, i, &trap);

      // If this breakpoint was found
      if (not error &&
          (not misc_chararr_sub_to_int(4, address, trap.addressA))) {
        breakpoint_found = TRUE;
        trap_set_status(0, 0, 1 << i);
      }
    }
  }

  // breakpoint(s) matched and deleted ???
  if (breakpoint_found) {
    std::cout << "turning " << addr << " OFF!" << std::endl;
    return false;
  }
  // See if we can set breakpoint
  else {
    std::cout << "turning " << addr << " ON!" << std::endl;
    temp = (~worda) & wordb;  // Undefined => possible choice

    // If any free entries ...
    if (temp != 0) {  // Define (set) breakpoint
      trap_def trap;
      int count, i;

      for (i = 0; (((temp >> i) & 1) == 0); i++)
        ;

      // Choose free(?) number
      trap.misc = -1;  // Really two byte parameters

      // Should send 2*words address then two*double words data @@@
      view_chararrCpychararr(8, address, trap.addressA);
      for (count = 0; count < 4; count++) {
        trap.addressB[count] = 0xFF;
      }
      for (count = 0; count < 8; count++) {
        trap.dataA[count] = 0x00;
        trap.dataB[count] = 0x00;
      }

      write_trap_defn(0, i, &trap);
      return true;
    } else {
      return false;
    }
  }
}

/**
 * @brief Check the state of the board - logs what it is doing.
 * @return int 0 if the board is in a failed state. else 1.
 */
const int Jimulator::checkBoardState() {
  const int board_state = board_enq();

  printf("STATE CODE: %X - ", board_state);

  // Check and log error states
  switch (board_state) {
    case CLIENT_STATE_RUNNING_SWI:
      printf("⚠board running SWI\n");
      break;
    case CLIENT_STATE_RUNNING:
      printf("⚠board running\n");
      break;
    case CLIENT_STATE_STEPPING:
      printf("⚠board stepping\n");
      break;
    case CLIENT_STATE_MEMFAULT:
      printf("⚠board memory fault\n");
      break;
    case CLIENT_STATE_BUSY:
      printf("⚠board is busy\n");
      break;
    case CLIENT_STATE_BYPROG:
      printf("🎉Program execution finished\n");
      break;
    default:
      printf("😅nothing to report\n");
      return FALSE;
      break;
  }

  return board_state;
}

/**
 * @brief Queries a register in Jimulator to get it's current value.
 * @return The values read from the registers.
 */
const std::array<std::string, 16> Jimulator::getJimulatorRegisterValues() {
  unsigned char regVals[64];            // 64-byte array
  readRegistersIntoArray(regVals, 16);  // regVals now has reg values

  std::array<std::string, 16> a;  // vector of strings

  // Loop through the array
  for (long unsigned int j = 0; j < a.size(); j++) {
    a[j] = view_chararr2hexstrbe(4, &regVals[j * 4], true);
  }

  return a;
}

const std::string Jimulator::getJimulatorTerminalMessages() {
  unsigned char
      string[256];  // (large enough) string to get the message from the board
  unsigned char length;

  std::string output("");

  do {
    boardSendChar(BR_FR_READ);  // send appropriate command
    boardSendChar(0);           // send the terminal number
    boardSendChar(32);          // send the maximum length possible
    boardGetChar(&length);      // get length of message

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
    boardSendChar(BR_FR_WRITE);  // begins a write
    boardSendChar(0);            // tells where to send it
    boardSendChar(1);            // send length 1
    boardSendChar(key_pressed);  // send the message - 1 char currently
    boardGetChar(&res);          // Read the result
    return true;
  }

  return false;
}

/**
 * @brief Get the memory values from Jimulator, starting to s_address_int.
 * @param s_address_int The address to start at, as an integer.
 * @return std::array<Jimulator::MemoryValues, 15> An array of all of the
 * values read from Jimulator, including each column.
 */
std::array<Jimulator::MemoryValues, 15> Jimulator::getJimulatorMemoryValues(
    const uint32_t s_address_int) {
  const int count = 15;  // The number of values displayed in the memory window
  const int bytecount = 60;  // The number of bytes to fetch from memory

  // Bit level hacking happening here - converting the integer address into
  // an array of characters.
  unsigned char* p = (unsigned char*)&s_address_int;
  unsigned char start_address[4] = {p[0], p[1], p[2], p[3]};
  start_address[0] &= -4;  // Normalise down to the previous multiple of 4

  unsigned char memdata[bytecount];

  // Reading data into arrays!
  boardSendChar(72 | 2);
  boardSendCharArray(4, start_address);
  board_send_n_bytes(count, 2);
  boardGetCharArray(bytecount, memdata);

  // ! Dangerous old logic ahead is used to read memory.
  // ! This is very much C-style code, be careful

  // Switches the endianness of the address
  unsigned char address[4] = {start_address[0], start_address[1],
                              start_address[2], start_address[3]};

  source_line* src = NULL;
  bool used_first = false;
  unsigned int start_addr = view_chararr2int(4, start_address);

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
  std::array<Jimulator::MemoryValues, 15> readValues;
  auto bps = getAllBreakpoints();

  // Iterate over display rows
  for (int row = 0; row < 15; row++) {
    int increment = 4;

    unsigned int addr = view_chararr2int(4, address);
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
                view_chararr2hexstrbe(src->data_size[i],
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
        readValues[row].disassembly = std::string(src->text);

        g_free(pStr);

        do {
          if (src->pNext != NULL) {
            src = src->pNext; /* Move on ... */
          } else {            /* ... wrapping, if required */
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
        increment = source_disassemble(src, addr, increment);
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
    view_chararrAdd(4, address, increment, address);
  }

  return readValues;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!! Functions below are not included in the header file !!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

/**
 * @brief Gets the status of a trap.
 * @param trap_type
 * @param wordA
 * @param wordB
 * @return true
 * @return false
 */
bool trap_get_status(unsigned int trap_type,
                     unsigned int* wordA,
                     unsigned int* wordB) {
  boardSendChar(BR_BP_GET | ((trap_type << 2) & 0xC));  // get breakpoint packet
  return ((board_get_n_bytes((int*)wordA, 4) != 4) ||
          (board_get_n_bytes((int*)wordB, 4) != 4));
}

/**
 * @brief Reads a trap definition for use by the breakpoints callbacks.
 * @param trap_type
 * @param trap_number
 * @param trap_defn
 * @return boolean
 */
bool read_trap_defn(unsigned int trap_type,
                    unsigned int trap_number,
                    trap_def* trap_defn) {
  boardSendChar(BR_BP_READ | ((trap_type << 2) & 0xC));  // send trap-read
                                                         // packet
  boardSendChar(trap_number);  // send the number of the definition

  if ((2 != board_get_n_bytes((int*)&((*trap_defn).misc),
                              2))  // get the trap misc properties
      || (4 != boardGetCharArray(4, (*trap_defn).addressA)) ||
      (4 != boardGetCharArray(4, (*trap_defn).addressB)) ||
      (8 != boardGetCharArray(8, (*trap_defn).dataA)) ||
      (8 != boardGetCharArray(8, (*trap_defn).dataB))) {
    // get address a&b and data a&b
    return FALSE;
  } else
    return TRUE;
}

int misc_chararr_sub_to_int(int count,
                            unsigned char* value1,
                            unsigned char* value2) {
  int ret = 0; /* bit array - bit array => int */

  while (count--) {
    ret = (ret << 8) + (int)value1[count] - (int)value2[count];
  }
  return ret;  // Carry propagated because intermediate -int- can be negative
}

void trap_set_status(unsigned int trap_type,
                     unsigned int wordA,
                     unsigned int wordB) {
  boardSendChar(BR_BP_SET |
                ((trap_type << 2) & 0xC)); /* set breakpoint packet */
  board_send_n_bytes(wordA, 4);            /* send word a */
  board_send_n_bytes(wordB, 4);            /* send word b */
  return;
}

/**
 * @brief Copy count characters from source to destination
 *
 * @param count
 * @param source
 * @param destination
 */
void view_chararrCpychararr(int count,
                            unsigned char* source,
                            unsigned char* destination) {
  while (count--) {
    destination[count] = source[count]; /* Copy char array */
  }
}

bool write_trap_defn(unsigned int trap_type,
                     unsigned int trap_number,
                     trap_def* trap_defn) {
  boardSendChar(BR_BP_WRITE | ((trap_type << 2) & 0xC));
  // send trap-write packet
  boardSendChar(trap_number);  // send the number of the definition
  board_send_n_bytes(((*trap_defn).misc), 2);  // send the trap misc properties
  boardSendCharArray(4, (*trap_defn).addressA);
  boardSendCharArray(4, (*trap_defn).addressB);
  boardSendCharArray(8, (*trap_defn).dataA);
  boardSendCharArray(8, (*trap_defn).dataB);  // send address a&b and data a&b
  return TRUE;
}

/**
 * @brief Sends char_number bytes located at data_ptr to current client
 * @param char_number The number of bytes to send from data_ptr.
 * @param data_ptr The data to send.
 * @return int number of bytes transmitted (currently always same as input)
 */
int boardSendCharArray(int char_number, unsigned char* data_ptr) {
  struct pollfd pollfd;
  pollfd.fd = writeToJimulator;
  pollfd.events = POLLOUT;

  // See if output possible
  if (not poll(&pollfd, 1, OUT_POLL_TIMEOUT)) {
    printf("Client system not responding!\n");  // Warn; poss. communication
                                                // problem
  }

  // Write char_number bytes
  if (write(writeToJimulator, data_ptr, char_number) == -1) {
    printf("Pipe write error!\n");
  }

  return char_number;
}

/**
 * @brief send a single character/byte to the board using the general case of
 * sending an array of bytes.
 * @param to_send
 * @return int
 */
const int boardSendChar(unsigned char to_send) {
  return boardSendCharArray(1, &to_send);
}

/**
 * @brief Gets a character array from the board.
 * @param char_number
 * @param data_ptr
 * @return int Number of bytes received, up to char_number number of
 * characters in array at data_ptr.
 */
int boardGetCharArray(int char_number, unsigned char* data_ptr) {
  int reply_count; /* Number of chars fetched in latest attempt */
  int reply_total; /* Total number of chars fetched during invocation */
  struct pollfd pollfd;

  pollfd.fd = readFromJimulator;
  pollfd.events = POLLIN;
  reply_total = 0;

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
 * @brief get a single character/byte from the board using the general case of
 * getting an array of bytes.
 * @param to_get
 * @return int
 */
int boardGetChar(unsigned char* to_get) {
  return boardGetCharArray(1, to_get);
}

/**
 * @brief Gets N bytes from the board into the indicated val_ptr, LSB first.
 * If error suspected sets `board_version' to not present
 * @param val_ptr
 * @param n
 * @return int The number of bytes received successfully (i.e. N=>"Ok")
 */
int board_get_n_bytes(int* val_ptr, int n) {
  if (n > MAX_SERIAL_WORD) {
    n = MAX_SERIAL_WORD;  // Clip, just in case
  }

  char unsigned buffer[MAX_SERIAL_WORD];
  int numberOfReceivedBytes = boardGetCharArray(n, buffer);
  *val_ptr = 0;

  for (int i = 0; i < numberOfReceivedBytes; i++) {
    *val_ptr = *val_ptr | ((buffer[i] & 0xFF) << (i * 8));  // Assemble integer
  }

  return numberOfReceivedBytes;
}

/**
 * @brief Find the current execution state of the client
 * @return int client response if stopped, running, error, else "BROKEN"
 */
const int board_enq() {
  unsigned char clientStatus = 0;
  int stepsSinceReset;
  int leftOfWalk;

  // If the board sends back the wrong the amount of data
  boardSendChar(BR_WOT_U_DO);

  if (boardGetChar(&clientStatus) != 1 ||
      board_get_n_bytes(&leftOfWalk, 4) != 4 ||       // Steps remaining
      board_get_n_bytes(&stepsSinceReset, 4) != 4) {  // Steps since reset
    printf("board not responding\n");
    return CLIENT_STATE_BROKEN;
  }

  // TODO: clientStatus represents what the board is doing and why - can be
  // reflected in the view?

  std::cout << "Total steps: " << stepsSinceReset << std::endl;

  return clientStatus;
}

/**
 * @brief Sends N bytes from the supplied value to the board, LSB first.
 * @param value The values to send.
 * @param n The number of bytes.
 * @return int The number of bytes believed received successfully (i.e.
 * N=>"Ok")
 */
int board_send_n_bytes(int value, int n) {
  unsigned char buffer[MAX_SERIAL_WORD];

  if (n > MAX_SERIAL_WORD) {
    n = MAX_SERIAL_WORD;  // Clip, just in case...
  }

  for (int i = 0; i < n; i++) {
    buffer[i] = value & 0xFF;  // Byte into buffer
    value = value >> 8;        // Get next byte
  }

  return boardSendCharArray(n, buffer);
}

/**
 * @brief Signals the board to run for `steps` number of steps.
 * @param steps The number of steps to run for (0 if indefinite)
 */
void run_board(const int steps) {
  if (not Jimulator::checkBoardState()) {
    // Sends a start signal &  whether breakpoints are on
    boardSendChar(BR_START | 48);

    board_send_n_bytes(steps, 4);  // Send step count
  }
}

/**
 * @brief Reads register information from the board.
 * @param data The pointer to read the register values into.
 */
void readRegistersIntoArray(unsigned char* data, unsigned int count) {
  boardSendChar(BR_GET_REG);
  board_send_n_bytes(0, 4);
  board_send_n_bytes(16, 2);
  boardGetCharArray(64, data);
}

/**
 * @brief Reads the values from a char array into a C++ string.
 * @warning This function does some bit-level trickery. This is very low
 * level, study carefully before altering control flow.
 * @param count The number of bytes to read out.
 * @param values The particular register value to read.
 * @param prepend Whether to prepend the output string with an "0x" or not.
 * @return std::string A hexadecimal formatted register value.
 */
std::string view_chararr2hexstrbe(int count,
                                  unsigned char* values,
                                  bool prepend) {
  char ret[count * 2 + 1];
  char* ptr = ret;

  while (count--) {
    g_snprintf(ptr, 3, "%02X", (int)values[count]);
    ptr += 2;  // Step string pointer
  }

  if (prepend) {
    return std::string(ret).insert(0, "0x");
  } else {
    return std::string(ret);
  }
}

/**
 * @brief Converts a bit array into an integer output.
 * @param count
 * @param array
 * @return int The converted array.
 */
int view_chararr2int(int count, unsigned char* array) {
  int ret = 0; /* bit array => int */

  while (count--) {
    ret = (ret << 8) + (array[count] & 0xFF);
  }
  return ret;
}

/**
 * @brief "acc" may be the same as "byte_string"
 * @param count
 * @param byte_string
 * @param number
 * @param acc
 */
void view_chararrAdd(int count,
                     unsigned char* byte_string,
                     int number,
                     unsigned char* acc) {
  int index, temp;

  for (index = 0; index < count; index++) {
    temp = byte_string[index] + (number & 0xFF); /* Add next 8 bits */
    acc[index] = temp & 0xFF;
    number = number >> 8; /* Shift to next byte */
    if (temp >= 0x100)
      number++; /* Propagate carry */
  }
  return;
}

int source_disassemble(source_line* src, unsigned int addr, int increment) {
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
  unsigned int worda, wordb;
  bool error = FALSE;

  // Have trap status info okay: get breaks and breaksinactive list
  if (not trap_get_status(0, &worda, &wordb)) {
    // Loop through every breakpoint
    for (int i = 0; (i < 32) && not error; i++) {
      // If breakpoint found, read the breakpoint value
      if (((worda >> i) & 1) != 0) {
        trap_def trap;

        // if okay
        if (read_trap_defn(0, i, &trap)) {
          // Get address and save into map
          u_int32_t addr = view_chararr2int(4, trap.addressA);
          breakpointAddresses.insert({addr, true});
        } else {
          error = TRUE;  // Read failure causes loop termination
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
void flush_source() {
  source_line *pOld, *pTrash;

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
const int check_hex(const char character) {
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
 * @brief Read hex number and return bytes (2^N) also
 * @param fHandle
 * @param pC
 * @param number
 * @return int
 */
const int get_number(FILE* const fHandle,
                     char* const pC,
                     unsigned int* const number) {
  while ((*pC == ' ') || (*pC == '\t')) {
    *pC = getc(fHandle);  // Skip spaces
  }

  int j = 0, value = 0;
  for (int digit = check_hex(*pC); digit >= 0; digit = check_hex(*pC), j++) {
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
 * @brief determines if a character is "allowed"
 * @param c the character to determine.
 * @return int
 */
const int allowed(const char c) {
  return (c == '_') || ((c >= '0') && (c <= '9')) ||
         (((c & 0xDF) >= 'A') && ((c & 0xDF) <= 'Z'));
}

/**
 * @brief Look-up function to encode length of memory transfer: Only four
 * legal sizes at present.
 * @param size
 * @return unsigned int
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
 * @return TRUE for success, FALSE for failure
 */
const int boardSetMemory(const int count,
                         unsigned char* const address,
                         unsigned char* const value,
                         const int size) {
  const int bytecount = count * size;

  if ((1 != boardSendChar(BR_SET_MEM | boardTranslateMemsize(size))) ||
      (4 != boardSendCharArray(4, address))                      // send address
      || (2 != board_send_n_bytes(count, 2))                     // send width
      || (bytecount != boardSendCharArray(bytecount, value))) {  // send value
    printf("bad board version!\n");
    return FALSE;
  }

  return TRUE;  // Copy of global indicates success/failure
}

/**
 * @brief Reads the source of the file pointer to by pathToKMD
 * @param pathToKMD A path to the `.kmd` file to be loaded.
 * @return int status code (1 is failure, 0 is success)
 */
const int readSource(const char* pathToKMD) {
  // TODO: this function is a jumbled mess. refactor and remove sections
  unsigned int address, old_address;
  unsigned int d_size[SOURCE_FIELD_COUNT], d_value[SOURCE_FIELD_COUNT];
  int i, j, m, flag;
  int byte_total, text_length;
  char c, buffer[SOURCE_TEXT_LENGTH + 1]; /* + 1 for terminator */
  source_line *pNew, *pTemp1, *pTemp2;
  struct stat status;

  // `system` runs the paramter string as a shell command (i.e. it launches a
  // new process). `pidof` checks to see if a process by the name `jimulator` is
  // running. If it fails (non-zero) It will print an error and return failure.
  if (system("pidof -x jimulator > /dev/null")) {
    // TODO: Jimulator is not running... so relaunch Jimulator?
    printf("Jimulator is not running!\n");
    return 1;
  }

  FILE* komodoSource = fopen(pathToKMD, "r");
  if (komodoSource == NULL) {
    printf("Source could not be opened!\n");
    return 1;
  }

  int has_old_addr = FALSE;  // Don't know where we start
  stat(pathToKMD, &status);

  // Repeat until end of file
  while (not feof(komodoSource)) {
    address = 0;   // Really needed?
    flag = FALSE;  // Haven't found an address yet
    c = getc(komodoSource);

    // If the first character is a colon, read a symbol record
    if (c == ':') {
      has_old_addr = FALSE;  // Don't retain position
    }

    // Read a source line record
    else {
      for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
        d_size[j] = 0;
        d_value[j] = 0;
      }

      byte_total = 0;
      flag = get_number(komodoSource, &c, &address) != 0;  // Some digits read?

      // Read a new address - and if we got an address, try for data fields
      if (flag) {
        if (c == ':') {
          c = getc(komodoSource);  // Skip colon
        }

        // Loop on data fields
        // repeat several times or until `illegal' character met
        for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
          d_size[j] = get_number(komodoSource, &c, &d_value[j]);

          if (d_size[j] == 0) {
            break;  // Quit if nothing found
          }

          byte_total = byte_total + d_size[j];  // Total input
        }

        old_address = address + byte_total;  // Predicted -next- address
        has_old_addr = TRUE;
      }
      // Address field not found  Maybe something useable?
      else if (has_old_addr) {
        address = old_address;  // Use predicted address
        flag = TRUE;            // Note we do have an address
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

          buffer[text_length++] = '\0';  // text_length now length incl. '\0'
          pNew = g_new(source_line, 1);  // Create new record
          pNew->address = address;
          pNew->corrupt = FALSE;

          byte_total = 0;  // Inefficient
          for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
            pNew->data_size[j] = d_size[j];  // Bytes, not digits
            pNew->data_value[j] = d_value[j];

            // clips memory load - debatable
            if ((pNew->data_size[j] > 0) &&
                ((pNew->data_size[j] + byte_total) <= SOURCE_BYTE_COUNT)) {
              unsigned char addr[4], data[4];

              for (i = 0; i < 4; i++) {
                addr[i] = ((address + byte_total) >> (8 * i)) & 0xFF;
              }
              for (i = 0; i < pNew->data_size[j]; i++) {
                data[i] = (pNew->data_value[j] >> (8 * i)) & 0xFF;
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
            printf("OVERFLOW %d %d %d %d  %d\n", d_size[0], d_size[1],
                   d_size[2], d_size[3], byte_total);
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
  return 0;
}
