cmake_minimum_required(VERSION 2.8.0)
project(sample)

#set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
#include_directories(include)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

list(APPEND SOURCES sample.cpp)
list(APPEND SOURCES stdafx.h)

if(MSVC)
   set_source_files_properties(stdafx.cpp PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
   set_source_files_properties(sample.cpp PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
endif()

list(APPEND SOURCES stdafx.cpp)

add_executable(Sample ${SOURCES})