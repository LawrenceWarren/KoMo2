cmake_minimum_required(VERSION 3.0.0)
project(KoMo2 VERSION 0.1.0 DESCRIPTION "An ARM emulation GUI." LANGUAGES C 
        HOMEPAGE_URL "https://github.com/LawrenceWarren")

# Use the package PkgConfig to detect GTK3+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK3+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
SET ( GTK3_LIBS ${GTK3_LIBRARIES} )

LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

# include(CTest)
# enable_testing(False)

add_executable(kmd src/callbacks.c src/main.c)
TARGET_LINK_LIBRARIES(kmd ${CMAKE_THREAD_LIBS_INIT} ${GTK3_LIBS} )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
