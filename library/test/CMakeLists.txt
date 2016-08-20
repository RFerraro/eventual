cmake_minimum_required(VERSION 3.0)
project(test)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(conan_arch "x86_64")
else()
   set(conan_arch "x86")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   set(conan_compiler "Visual Studio")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   set(conan_compiler "clang")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   set(conan_compiler "gcc")
else()
   set(conan_compiler ${CMAKE_CXX_COMPILER_ID})
   message(WARNING "WARNING: connan.io may not support compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

set(test_sources
    ./EventualTests.cpp
    ./FutureTests.cpp
    ./PackagedTaskTests.cpp
    ./PolymorphicAllocatorTests.cpp
    ./PromiseTests.cpp
    ./ResourceAdapterTests.cpp
    ./SharedFutureTests.cpp
    ./StrongPolymorphicAllocatorTests.cpp
    ./test.cpp
    ./stdafx.cpp
    ./stdafx.h
    ./BasicAllocator.h
    ./NullResource.h
    ./FutureTestPatterns.h
    ./NonCopyable.h)

add_executable(Test ${test_sources})

target_link_libraries(Test PUBLIC eventual_lib)
target_include_directories(Test PUBLIC ../)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   # Because Conan.io (0.10.0) lives in a fantasy world where they only think that visual studio only needs a "single-configuration"...

   add_custom_command(
      TARGET Test PRE_BUILD
      COMMAND conan install ${CMAKE_SOURCE_DIR} -s arch=${conan_arch} -s build_type=Release -s compiler="${conan_compiler}" -s compiler.runtime=MT -build=missing
      COMMAND cmake -E rename conanbuildinfo.cmake conanbuildinfo_release.cmake
      )
   
   add_custom_command(
      TARGET Test PRE_BUILD
      COMMAND conan install ${CMAKE_SOURCE_DIR} -s arch=${conan_arch} -s build_type=Debug -s compiler="${conan_compiler}" -s compiler.runtime=MTd -build=missing
      COMMAND cmake -E rename conanbuildinfo.cmake conanbuildinfo_debug.cmake 
      )
   
   include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_release.cmake)
   set(GTEST_INCLUDE_RELEASE ${CONAN_INCLUDE_DIRS_GTEST})
   set(GTEST_LIBDIR_RELEASE ${CONAN_LIB_DIRS_GTEST})
   set(GTEST_LIBS_RELEASE ${CONAN_LIBS_GTEST})
   
   include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_debug.cmake)
   set(GTEST_INCLUDE_DEBUG ${CONAN_INCLUDE_DIRS_GTEST})
   set(GTEST_LIBDIR_DEBUG ${CONAN_LIB_DIRS_GTEST})
   set(GTEST_LIBS_DEBUG ${CONAN_LIBS_GTEST})

   target_include_directories(Test SYSTEM PUBLIC $<$<CONFIG:Debug>:${GTEST_INCLUDE_DEBUG}> $<$<CONFIG:Release>:${GTEST_INCLUDE_RELEASE}>)
   target_link_libraries(Test PUBLIC "$<$<CONFIG:Debug>:${GTEST_LIBS_RELEASE}>" "$<$<CONFIG:Release>:${GTEST_LIBS_DEBUG}>")
   set_target_properties(Test PROPERTIES LINK_FLAGS_DEBUG "/LIBPATH:\"${GTEST_LIBDIR_DEBUG}\"")
   set_target_properties(Test PROPERTIES LINK_FLAGS_RELEASE "/LIBPATH:\"${GTEST_LIBDIR_RELEASE}\"")
   
   
   target_compile_options(Test PUBLIC
                          "/W4" 
                          "/WX" 
                          "$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:/bigobj>" 
                          "$<$<CONFIG:Debug>:/MTd>" 
                          "$<$<CONFIG:Release>:/MT>")
   set_target_properties(Test PROPERTIES LINK_FLAGS "/WX")
else()
   #clang/gcc...
   
   add_custom_command(
      TARGET Test PRE_BUILD
      COMMAND conan install ${CMAKE_SOURCE_DIR} -s arch=${conan_arch} -s build_type=${CMAKE_BUILD_TYPE} -s compiler="${conan_compiler}" -s compiler.libcxx="libstdc++11" -build=missing
      )

   set(CONAN_SYSTEM_INCLUDES True)
   include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
   conan_basic_setup()
   
   target_link_libraries(Test PUBLIC ${CONAN_LIBS})
   
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   
   target_compile_options(Test PUBLIC
                          "-std=c++14" 
                          "-Werror" 
                          "-Wall"
                          "-Wextra"
                          "-Wunreachable-code"
                          "$<$<CONFIG:Debug>:-g>"
                          "$<$<CONFIG:Debug>:-O0>"
                          "$<$<CONFIG:Debug>:--coverage>")
   set_target_properties(Test PROPERTIES LINK_FLAGS_DEBUG "--coverage")
   
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

   target_compile_options(Test PUBLIC
                          "-std=c++14" 
                          "-Werror" 
                          "-Wall"
                          "-Wextra"
                          "-Wunreachable-code"
                          "$<$<CONFIG:Debug>:-g>"
                          "$<$<CONFIG:Debug>:-O0>"
                          "$<$<CONFIG:Debug>:--coverage>"
                          "$<$<CONFIG:Debug>:-fno-inline>"
                          "$<$<CONFIG:Debug>:-fno-inline-small-functions>"
                          "$<$<CONFIG:Debug>:-fno-default-inline>"
                          )
   set_target_properties(Test PROPERTIES LINK_FLAGS_DEBUG "--coverage")
   
endif()

