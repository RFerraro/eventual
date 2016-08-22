cmake_minimum_required(VERSION 3.0)

include(GenerateExportHeader)
project(eventual)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(eventual_headers
    ./eventual.h
    ./detail/allocation.h
    ./detail/implementation.h
    ./detail/traits.h
    ./detail/utility.h)

#dummy target
add_custom_target(eventual SOURCES ${eventual_headers})

#header only
add_library(eventual_lib INTERFACE)
target_compile_definitions(
    eventual_lib
    INTERFACE LIBRARY_HEADER_ONLY
)

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
