cmake_minimum_required(VERSION 2.8.0)
project(test)

#set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
#include_directories(include)

if(MSVC)
   
   if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      # 64bit build
      include(${CMAKE_SOURCE_DIR}/conanbuildinfo_release_x86_64.cmake)
   else()
      include(${CMAKE_SOURCE_DIR}/conanbuildinfo_release_x86.cmake)
   endif()
   
   set(GTEST_INCLUDE_RELEASE ${CONAN_INCLUDE_DIRS_GTEST})
   set(GTEST_LIBDIR_RELEASE ${CONAN_LIB_DIRS_GTEST})
   set(GTEST_LIBS_RELEASE ${CONAN_LIBS_GTEST})
   
   if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      # 64bit build
      include(${CMAKE_SOURCE_DIR}/conanbuildinfo_debug_x86_64.cmake)
   else()
      include(${CMAKE_SOURCE_DIR}/conanbuildinfo_debug_x86.cmake)
   endif()
      
   set(GTEST_INCLUDE_DEBUG ${CONAN_INCLUDE_DIRS_GTEST})
   set(GTEST_LIBDIR_DEBUG ${CONAN_LIB_DIRS_GTEST})
   set(GTEST_LIBS_DEBUG ${CONAN_LIBS_GTEST})
   
   include_directories($<$<CONFIG:Debug>:${GTEST_INCLUDE_DEBUG}> $<$<CONFIG:Release>:${GTEST_INCLUDE_RELEASE}>)
   
else()
   include(${CMAKE_SOURCE_DIR}/conanbuildinfo.cmake)
   conan_basic_setup()
endif()

list(APPEND TEST_SOURCES EventualTests.cpp)
list(APPEND TEST_SOURCES FutureTests.cpp)
list(APPEND TEST_SOURCES PackagedTaskTests.cpp)
list(APPEND TEST_SOURCES PromiseTests.cpp)
list(APPEND TEST_SOURCES SharedFutureTests.cpp)
list(APPEND TEST_SOURCES test.cpp)

if(MSVC)
   
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Yu\"stdafx.h\"")

   set_source_files_properties(stdafx.cpp
      PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
      
   foreach(src_file ${TEST_SOURCES})
      set_source_files_properties(${src_file}
         PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
   endforeach()
   
   set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /LIBPATH:\"${GTEST_LIBDIR_DEBUG}\"")
   set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LIBPATH:\"${GTEST_LIBDIR_RELEASE}\"")

endif()

list(APPEND TEST_SOURCES stdafx.cpp)
list(APPEND TEST_SOURCES stdafx.h)
list(APPEND TEST_SOURCES BasicAllocator.h)
list(APPEND TEST_SOURCES FutureTestPatterns.h)
list(APPEND TEST_SOURCES NonCopyable.h)
list(APPEND TEST_SOURCES "../include/eventual/eventual.h")

add_executable(Test ${TEST_SOURCES})

if(MSVC)
   #listing generates duplicates for some reason...
   foreach(lib_item ${GTEST_LIBS_RELEASE})
      target_link_libraries(Test optimized ${lib_item})
   endforeach()

   #listing generates duplicates for some reason...
   foreach(lib_item ${GTEST_LIBS_DEBUG})
      target_link_libraries(Test debug ${lib_item})
   endforeach()
else()
   target_link_libraries(Test ${CONAN_LIBS})
endif()

