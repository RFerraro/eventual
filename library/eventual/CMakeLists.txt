cmake_minimum_required(VERSION 3.0)
include(GenerateExportHeader)

project(eventual)

set(eventual_headers
    ./eventual.h
    ./detail/allocation.h
    ./detail/implementation.h
    ./detail/traits.h
    ./detail/utility.h)

add_library(eventual ${eventual_headers})
set_target_properties(eventual PROPERTIES LINKER_LANGUAGE CXX)
generate_export_header(eventual)

#add_custom_target(eventual SOURCES ${eventual_headers})

#header only
add_library(eventual_lib INTERFACE)
target_compile_definitions(
    eventual_lib
    INTERFACE LIBRARY_HEADER_ONLY
)

#target_compile_definitions(eventual INTERFACE LIBRARY_HEADER_ONLY)
	
#add_library(library_static STATIC ${library_sources})
#add_library(library_shared SHARED ${library_sources})

#add_library(eventual ${eventual_headers})
#set_target_properties(eventual PROPERTIES LINKER_LANGUAGE CXX)


#generate_export_header(library_iface)
#set_target_properties(library_static PROPERTIES COMPILE_FLAGS -DEVENTUAL_STATIC_DEFINE)

#create an exportfile directed in the build tree (rather then installing)
EXPORT(
        TARGETS
            eventual_lib
        FILE
            eventual-tree-export.cmake)

install(
        DIRECTORY
            detail
        DESTINATION
            include/eventual)

install(
        FILES
            eventual.h
            ${CMAKE_CURRENT_BINARY_DIR}/eventual_export.h
        DESTINATION
            include/eventual)
			
install(
        TARGETS
            eventual_lib
        EXPORT eventual
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION include)
		
install(
        EXPORT eventual
        NAMESPACE eventual::
        DESTINATION lib/cmake/eventual)
		
#install(
#        FILES eventual-config.cmake
#        DESTINATION lib/cmake/eventual)
