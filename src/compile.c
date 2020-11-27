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
