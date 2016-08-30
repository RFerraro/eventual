cmake_minimum_required(VERSION 3.0)
project(Eventual)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HEADER_ONLY "Generate a header-only library")

if(CMAKE_CONFIGURATION_TYPES)
   # Reduce the config types to only Debug and Release
   set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE STRING "" FORCE)

add_subdirectory(library/eventual)
add_subdirectory(library/sample)
add_subdirectory(library/test)

