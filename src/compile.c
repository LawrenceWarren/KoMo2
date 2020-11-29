/**
 * @file compile.c
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Contains functions related to the compilation of ARM assembly `.s` 
 * files to KoMoDo readable `.kmd` files, and their subsequent loading into
 * the Jimulator ARM emulator.
 * @version 0.1
 * @date 2020-11-27
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "compile.h"

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

#include "globals.h"

// Forward declaring auxiliary load functions
int misc_count_symbols();
void flush_sauce();
void misc_flush_symbol_table();
int read_sauce(const char *pathToKMD);
int callback_memory_refresh();
source_file source;

/**
 * @brief Stolen from the original KMD source - runs the file specified by
 * `pathToFile` through the KoMoDo compile script.
 * @param pathToBin An absolute path to the `aasm` binary.
 * @param pathToS An absolute path to the `.s` file to be compiled.
 * @param pathToKMD an absolute path to the `.kmd` file that will be output.
 */
int compile(const char *pathToBin, const char *pathToS, const char *pathToKMD) {
  printf("%s\n", pathToBin);
  printf("%s\n", pathToS);
  printf("%s\n", pathToKMD);
  execlp(pathToBin, "-lk", pathToS, pathToKMD, (char *)0);

  // Should not get here!
  printf("Running ARM assembler failed. \n");
  return 1;
}

/**
 * @brief A proven working version of load_data. This is legacy code from 
 * KoMoDo, stripped down to disinclude anything related to serial ports, 
 * networking, etc - it is pure emulation.
 * @param pathToKMD an absolute path to the `.kmd` file that will be loaded.
 */
void load(const char *pathToKMD) {
    printf("load\n");
  static int load_lock = 0;  // Mutex

  // if nothing else is already loading
  if (load_lock == 0) {
    load_lock = 1;
    int old_symbol_count = misc_count_symbols();  // Remember old rows

    flush_sauce();
    misc_flush_symbol_table();
    read_sauce(pathToKMD);  // TODO: may need some failure catching

    // TODO: update views here - can use old_symbol_count
    load_lock = 0;
  }

  return;
}

/**
 * @brief Returns the global variable `symbol_count`
 * @return int `symbol_count`
 */
int misc_count_symbols(void) {
    printf("misc_count_symbols\n");
    return symbol_count;
}

/**
 * @brief removes all of the old references to the previous file.
 */
void flush_sauce(void) {
    printf("flush_sauce\n");
    source_line *pOld, *pTrash;

    pOld = source.pStart;
    source.pStart = NULL;
    source.pEnd = NULL;  // Backward links never used in anger @@@

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
 * @brief Deletes old symbol table data and resets the pointers.
 */
void misc_flush_symbol_table(void) {
    printf("misc_flush_symbol_table\n");
    symbol *pTrash;
    symbol *pSym = symbol_table;

    // For all symbols
    while (pSym != NULL) {
        g_free(pSym->name);  // Free name string
        pTrash = pSym;       // Keep pointer
        pSym = pSym->pDef;   // Move along the table
        g_free(pTrash);      // Free symbol entry
    }

    // Reset the symbol table
    symbol_table = NULL;
    symbol_count = 0;
}

/**
 * @brief Check if the passed character represents a legal hex digit.
 * @param character the character under test
 * @return int the hex value or -1 if not a legal hex digit
 */
int check_hex(char character) {
    printf("check_hex\n");
    if ((character >= '0') && (character <= '9'))
        return character - '0';
    else if ((character >= 'A') && (character <= 'F'))
        return character - 'A' + 10;
    else if ((character >= 'a') && (character <= 'f'))
        return character - 'a' + 10;
    // Ignore prefix characters AT ANY POINT   @@@ (for JNZ)
    else
        return -1;
}

/**
 * @brief Read hex number and return #bytes (2^N) also
 * @param fHandle 
 * @param pC 
 * @param number 
 * @return int 
 */
int get_number(FILE *fHandle, char *pC, unsigned int *number) {
    printf("get_number\n");
    while ((*pC == ' ') || (*pC == '\t')) {
        *pC = getc(fHandle);  // Skip spaces
    }

    int j = 0, value = 0;
    for (int digit = check_hex(*pC); digit >= 0; digit = check_hex(*pC), j++) {
        value = (value << 4) | digit;  // Accumulate digit
        *pC = getc(fHandle);           // Get next character
    }                                  // Exits at first non-hex character (which is discarded)

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
int allowed(char c) {
    printf("allowed\n");
    return (c == '_') || ((c >= '0') && (c <= '9')) || (((c & 0xDF) >= 'A') && ((c & 0xDF) <= 'Z'));
}

/**
 * @brief Adds a symbol to the symbol table.
 * @param name 
 * @param value 
 * @param sym_type 
 */
void misc_add_symbol(char *name, long value, symbol_type sym_type) {
    printf("misc_add_symbol\n");
    symbol *pSym, *pNew;

    pNew = g_new(symbol, 1);
    pNew->name = g_strdup(name);
    pNew->value = value;
    pNew->sym_type = sym_type;
    pNew->pDef = NULL;

    if (symbol_table == NULL) {
        symbol_table = pNew;
    } else {  // Append on list
        for (pSym = symbol_table; pSym->pDef != NULL; pSym = pSym->pDef) {
            ;
        }

        pSym->pDef = pNew;
    }

    symbol_count++;
}

/**
 * @brief Returns last character read (not really important!)
 * @param fHandle 
 * @return char 
 */
char get_symbol(FILE *fHandle) {
    printf("get_symbol\n");
    // Strip spaces
    char c;
    do {
        c = getc(fHandle);
    } while ((c == ' ') || (c == '\t'));

    // Copy string
    char buffer[SYM_BUF_SIZE];
    int i = 0;
    for (; allowed(c) && i < (SYM_BUF_SIZE - 1); i++) {
        buffer[i] = c;
        c = getc(fHandle);
    }

    buffer[i] = '\0';  // Terminate string

    while (allowed(c)) {
        c = getc(fHandle);  // Discard characters after limit
    }

    unsigned int value = 0;
    int defined = get_number(fHandle, &c, &value) > 0;

    while ((c == ' ') || (c == '\t')) {
        c = getc(fHandle);
    }

    symbol_type sym_type;
    if (!defined) {
        sym_type = SYM_UNDEFINED;  // Value missing
    } else {
        // Bodge - just first character
        switch (c) {
            case 'G':
                sym_type = SYM_GLOBAL;
                break;
            case 'L':
                sym_type = SYM_LOCAL;
                break;
            case 'U':
                sym_type = SYM_UNDEFINED;
                break;
            case 'V':
                sym_type = SYM_EQUATE;
                break;
            default:
                sym_type = SYM_NONE;
                break;
        }
    }

    misc_add_symbol(buffer, value, sym_type);  // Set symbol
    return c;                                  // ... and discard rest of line
}

void staticPrint() {
  static int x = 0;
  printf("%d\n", x++);
}

/**
 * @brief Sends char_number bytes located at data_ptr to current client
 * @param char_number 
 * @param data_ptr 
 * @return int number of bytes transmitted (currently always same as input)
 */
int board_sendchararray(int char_number, unsigned char *data_ptr) {
  printf("board_sendchararray\n");
  struct pollfd pollfd;
  int blocked_flag;

  pollfd.fd = writeToJimulator;
  pollfd.events = POLLOUT;
  blocked_flag = FALSE;  // Hasn't timed out

  // See if output possible
  staticPrint();
  while (poll(&pollfd, 1, OUT_POLL_TIMEOUT) == 0) {
    printf("in while\n");
    if (!blocked_flag) {
      printf("in if 1\n");
      printf("Client system not responding!\n");  // Warn; poss. comms. problem
      staticPrint();
      blocked_flag = TRUE;                        // Hasn't timed out
    }
  }

  staticPrint();

  if (blocked_flag) {
    printf("in if 2\n");
    g_print("Client system okay!\n");
  }

  staticPrint();

  // Write char_number bytes
  if (write(writeToJimulator, data_ptr, char_number) == -1) {
    staticPrint();
    printf("Pipe write error!\n");
  }

  staticPrint();
  printf("returning...\n");
  return char_number;
}

/**
 * @brief send a single character/byte to the board using the general case of 
 * sending an array of bytes.
 * @param to_send 
 * @return int 
 */
int board_sendchar(unsigned char to_send) {
    printf("board_sendchar\n");
    return board_sendchararray(1, &to_send);
}

/**
 * @brief Look-up function to encode length of memory transfer: Only four legal
 * sizes at present.
 * @param size 
 * @return unsigned int 
 */
unsigned int board_translate_memsize(int size) {
    printf("board_translate_memsize\n");
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
 * @brief Sends N bytes from the supplied value to the board, LSB first.
 * @param value 
 * @param N 
 * @return int the number of bytes believed received successfully (i.e. N=>"Ok")
 */
int board_sendbN(int value, int N) {
    printf("board_sendbN\n");
    unsigned char buffer[MAX_SERIAL_WORD];
    int i;

    if (N > MAX_SERIAL_WORD) {
        N = MAX_SERIAL_WORD;  // Clip, just in case ...
    }

    for (i = 0; i < N; i++) {
        buffer[i] = value & 0xFF;  // Byte into buffer
        value = value >> 8;        // Get next byte
    }

    return board_sendchararray(N, buffer);
}

/**
 * @brief Sets a memory value of a given address to a new value.
 * This code is LEGACY. It used to run with a check on `board_version`: however
 * `board_version` always passed the check due to the certainty of the
 * "hardware" run under the emulator; therefore the check has been removed.
 * @param count number of elements
 * @param address pointer to the address in bytes
 * @param source pointer to the new value to be stored
 * @param size width of current memory in bytes
 * @return TRUE for success, FALSE for failure
 */
int board_set_memory(int count, unsigned char *address, unsigned char *source,
                     int size) {
    printf("board_set_memory\n");
    int bytecount = count * size;

    if ((1 != board_sendchar(BR_SET_MEM | board_translate_memsize(size))) || (board->memory_ptr_width != board_sendchararray(board->memory_ptr_width, address)) /* send address */
        || (2 != board_sendbN(count, 2))                                                                                                                        /* send width */
        || (bytecount != board_sendchararray(bytecount, source))) {                                                                                             /* send value */
        printf("bad board version!\n");
        return FALSE;
    }

    return TRUE;  // Copy of global indicates success/failure
}

/**
 * @brief Reads the source of the file pointer to by filename
 * @param filename 
 * @return int 
 */
int read_sauce(const char *filename) {
    printf("read_sauce\n");
    unsigned int address, old_address;
    unsigned int d_size[SOURCE_FIELD_COUNT], d_value[SOURCE_FIELD_COUNT];
    int i, j, m, flag, has_old_addr;
    int byte_total, text_length;
    char c, buffer[SOURCE_TEXT_LENGTH + 1]; /* + 1 for terminator */
    source_line *pNew, *pTemp1, *pTemp2;
    struct stat status;

    FILE *sauce = fopen(filename, "r");
    if (sauce == NULL) {
        printf("Source could not be opened!\n");
        return FALSE;
    }

    has_old_addr = FALSE;  // Don't know where we start
    stat(filename, &status);

    // Repeat until end of file
    while (!feof(sauce)) {
        address = 0;   // Really needed?
        flag = FALSE;  // Haven't found an address yet
        c = getc(sauce);

        // If the first character is a colon, read a symbol record
        if (c == ':') {
            get_symbol(sauce);
            has_old_addr = FALSE;  // Don't retain position
        }

        // Read a source line record
        else {
            for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
                d_size[j] = 0;
                d_value[j] = 0;
            }

            byte_total = 0;
            flag = get_number(sauce, &c, &address) != 0;  // Some digits read?

            // Read a new address - and if we got an address, try for data fields
            if (flag) {
                if (c == ':') {
                    c = getc(sauce);  // Skip colon
                }

                // Loop on data fields
                // repeat several times or until `illegal' character met
                for (j = 0; j < SOURCE_FIELD_COUNT; j++) {
                    d_size[j] = get_number(sauce, &c, &d_value[j]);

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
                while ((c != ';') && (c != '\n') && !feof(sauce)) {
                    c = getc(sauce);
                }

                // Check for field separator
                if (c == ';') {
                    c = getc(sauce);
                    if (c == ' ') {
                        c = getc(sauce);  // Skip formatting space
                    }

                    text_length = 0;  // Measure (& buffer) source line

                    // Everything to end of line (or clip)
                    while ((c != '\n') && !feof(sauce) && (text_length < SOURCE_TEXT_LENGTH)) {
                        buffer[text_length++] = c;
                        c = getc(sauce);
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
                        if ((pNew->data_size[j] > 0) && ((pNew->data_size[j] + byte_total) <= SOURCE_BYTE_COUNT)) {
                            unsigned char addr[4], data[4];

                            for (i = 0; i < 4; i++) {
                                addr[i] = ((address + byte_total) >> (8 * i)) & 0xFF;
                            }
                            for (i = 0; i < pNew->data_size[j]; i++) {
                                data[i] = (pNew->data_value[j] >> (8 * i)) & 0xFF;
                            }

                            // Ignore Boolean error return value for now
                            board_set_memory(1, addr, data, pNew->data_size[j]);
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
                        printf("OVERFLOW %d %d %d %d  %d\n", d_size[0], d_size[1], d_size[2], d_size[3], byte_total);
                        // Extend with some more records here? @@@ (plant in memory, above,  also)
                    }

                    // Copy text to buffer
                    pNew->text = g_new(char, text_length);
                    for (j = 0; j < text_length; j++) {
                        pNew->text[j] = buffer[j];
                    }

                    pTemp1 = source.pStart;  // Place new record in address ordered list
                    pTemp2 = NULL;           // behind any earlier records with same address.

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

        while ((c != '\n') && !feof(sauce)) {
            c = getc(sauce);  // Next line anyway
        }
    }

    fclose(sauce);
    return TRUE;  // Return error flag
}
