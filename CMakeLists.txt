cmake_minimum_required(VERSION 2.8.0)
project(Eventual)

# Reduce the config types to only Debug and Release
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      # 64bit build
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    endif()
    
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall /WX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /WX")

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   #Clang 3.7
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror -Weverything")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror -Wall -Wextra -Wunreachable-code")
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
   set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
   
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   #GCC 5.2
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror -Wall -Wextra -Wunreachable-code")
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
   set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
   
endif()

add_subdirectory(src/sample)
add_subdirectory(test)
