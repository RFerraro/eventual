cmake_minimum_required(VERSION 3.0)
project(Eventual)

# Reduce the config types to only Debug and Release
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

add_subdirectory(library/eventual)
add_subdirectory(library/sample)
add_subdirectory(library/test)
