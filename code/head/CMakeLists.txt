# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_library(schemas
        ../schemas/communication.c  # Reference the communication.c file in the schemas directory
        ../schemas/communication.h)
project(head)
