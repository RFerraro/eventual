cmake_minimum_required(VERSION 3.0)
project(sample)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(sample_sources
    ./sample.cpp
    ./stdafx.h
    ./stdafx.cpp)

add_executable(Sample ${sample_sources})

target_link_libraries(Sample PUBLIC eventual_lib)
target_include_directories(Sample PUBLIC ../)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

   target_compile_options(Sample PUBLIC
                          "/W4" 
                          "/WX" 
                          "$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:/bigobj>" 
                          "$<$<CONFIG:Debug>:/MTd>" 
                          "$<$<CONFIG:Release>:/MT>")
   set_target_properties(Sample PROPERTIES LINK_FLAGS "/WX")

elseif((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
   
   target_compile_options(Sample PUBLIC
                          "-std=c++14" 
                          "-Werror" 
                          "-Wall"
                          "-Wextra"
                          "-Wunreachable-code")
   
endif()

# Workaround for GCC > 6.0;
# See Bug 80916- https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80916
if((CMAKE_BUILD_TYPE MATCHES "Release") AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0) AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0))
   
   target_compile_options(Sample PUBLIC
                          "-Wno-error=unused-function")
   
endif()

install(
        TARGETS
            Sample
        DESTINATION bin
        )

