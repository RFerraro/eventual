cmake_minimum_required(VERSION 2.8.0)
project(sample)

#set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
#include_directories(include)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

list(APPEND SOURCES sample.cpp)
list(APPEND SOURCES stdafx.h)
list(APPEND SOURCES "../../include/eventual/eventual.h")
list(APPEND SOURCES "../../include/eventual/detail/implementation.h")
list(APPEND SOURCES "../../include/eventual/detail/traits.h")
list(APPEND SOURCES "../../include/eventual/detail/utility.h")

if(MSVC)

   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Yu\"stdafx.h\"")

   set_source_files_properties(stdafx.cpp PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
   set_source_files_properties(sample.cpp PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
endif()

list(APPEND SOURCES stdafx.cpp)

add_executable(Sample ${SOURCES})