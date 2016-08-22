cmake_minimum_required(VERSION 3.0)

include(CMakeParseArguments)
project(test)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(conan_arch "x86_64")
   set(is_x86_64 1)
else()
   set(conan_arch "x86")
   set(is_x86_64 0)
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


function(install_dependencies)
   set(install_args SOURCE_DIR ARCH BUILD_TYPE COMPILER RUNTIME LIBCXX OUTPUT_FILE)
   cmake_parse_arguments(install_dependencies "" "${install_args}" "" ${ARGN} )
   
   if((DEFINED install_dependencies_RUNTIME) AND (NOT install_dependencies_RUNTIME STREQUAL ""))
      
      execute_process(
         COMMAND conan install 
            "${install_dependencies_SOURCE_DIR}" 
            -s arch=${install_dependencies_ARCH} 
            -s build_type=${install_dependencies_BUILD_TYPE} 
            -s compiler=${install_dependencies_COMPILER} 
            -s compiler.runtime=${install_dependencies_RUNTIME} 
            -build=missing
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
         RESULT_VARIABLE conan_result
         )
   elseif((DEFINED install_dependencies_LIBCXX) AND (NOT install_dependencies_LIBCXX STREQUAL ""))
   
      execute_process(
         COMMAND conan install 
            "${install_dependencies_SOURCE_DIR}" 
            -s arch=${install_dependencies_ARCH} 
            -s build_type=${install_dependencies_BUILD_TYPE} 
            -s compiler=${install_dependencies_COMPILER}
            -s compiler.libcxx=${install_dependencies_LIBCXX}
            -build=missing
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
         RESULT_VARIABLE conan_result
         )    
   
   endif()
   
   if(NOT conan_result EQUAL 0)
      message(FATAL_ERROR "Conan.io failed to install this project's dependencies, error: ${conan_result}")
   endif()
   
   if((DEFINED install_dependencies_OUTPUT_FILE) AND (NOT install_dependencies_OUTPUT_FILE STREQUAL ""))
   
      execute_process(
      COMMAND ${CMAKE_COMMAND} -E rename "conanbuildinfo.cmake" "${install_dependencies_OUTPUT_FILE}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      RESULT_VARIABLE rename_result
      )
      
      if(NOT rename_result EQUAL 0)
         message(FATAL_ERROR "Cmake failed to rename this project's dependencies import file: \"conanbuildinfo.cmake\", to: \"${install_dependencies_OUTPUT_FILE}\"; error: ${rename_result}")
      endif()
      
   endif()

endfunction(install_dependencies)



if(CMAKE_CONFIGURATION_TYPES) #Multi-Configuration

   # Because Conan.io (0.10.0) lives in a fantasy world where they only think that visual studio only needs a "single-configuration"...
   
   install_dependencies(
      SOURCE_DIR ${CMAKE_SOURCE_DIR}
      ARCH ${conan_arch}
      BUILD_TYPE "Release"
      COMPILER ${conan_compiler}
      RUNTIME "MT"
      OUTPUT_FILE "conanbuildinfo_release.cmake"
      )
   
   install_dependencies(
      SOURCE_DIR ${CMAKE_SOURCE_DIR}
      ARCH ${conan_arch}
      BUILD_TYPE "Debug"
      COMPILER ${conan_compiler}
      RUNTIME "MTd"
      OUTPUT_FILE "conanbuildinfo_debug.cmake"
      )
      
else()

   install_dependencies(
      SOURCE_DIR ${CMAKE_SOURCE_DIR}
      ARCH ${conan_arch}
      BUILD_TYPE ${CMAKE_BUILD_TYPE}
      COMPILER ${conan_compiler}
      LIBCXX "libstdc++11"
      )
   
   #todo: link both paths the same...
   set(CONAN_SYSTEM_INCLUDES True)
   include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
   conan_basic_setup()

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
target_include_directories(Test PUBLIC ${CMAKE_SOURCE_DIR}/library)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   
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
                          "$<$<BOOL:${is_x86_64}>:/bigobj>" 
                          "$<$<CONFIG:Debug>:/MTd>" 
                          "$<$<CONFIG:Release>:/MT>")
   set_target_properties(Test PROPERTIES LINK_FLAGS "/WX")
else()
   #clang/gcc...
   
   target_link_libraries(Test PUBLIC ${CONAN_LIBS})
   
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
   
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

   target_compile_options(Test PUBLIC
                          "$<$<CONFIG:Debug>:-fno-inline>"
                          "$<$<CONFIG:Debug>:-fno-inline-small-functions>"
                          "$<$<CONFIG:Debug>:-fno-default-inline>"
                          )
endif()

