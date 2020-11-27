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

#include <stdio.h>
#include <unistd.h>

/**
 * @brief Stolen from the original KMD source - runs the file specified by
 * `pathToFile` through the KoMoDo compile script.
 * @param pathToFile an absolute path (from root) to the .s file to be compiled.
 */
void compile(const char *pathToFile) {
    // TODO: make "execlp" use a non-absolute path to the compile script.
    execlp("/home/lawrencewarren/projects/KMD2/bin/kmd_compile", pathToFile,
           pathToFile, (char *)0);
}
