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

const std::array<std::string, 16> getJimulatorRegisterValues();
const int compileJimulator(const char* const pathToBin,
                           const char* const pathToS,
                           const char* const pathToKMD);
const int loadJimulator(const char* const pathToKMD);
const int checkBoardState();
void startJimulator(const int steps);
void pauseJimulator();
void continueJimulator();
void resetJimulator();
