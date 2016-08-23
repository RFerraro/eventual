cmake_minimum_required(VERSION 3.0)

include(GenerateExportHeader)
project(eventual)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(eventual_headers
    ./eventual.h
    ./eventual_methods.h
    ./future.h
    ./packaged_task.h
    ./promise.h
    ./shared_future.h)
    
set(eventual_headers_detail
    ./detail/BasicFuture.h
    ./detail/BasicPromise.h
    ./detail/BasicTask.h
    ./detail/CommonPromise.h
    ./detail/CompositeState.h
    ./detail/detail.h
    ./detail/FutureFactory.h
    ./detail/FutureHelper.h
    ./detail/memory_resource.h
    ./detail/polymorphic_allocator.h
    ./detail/resource_adapter.h
    ./detail/ResultBlock.h
    ./detail/SimpleDelegate.h
    ./detail/State.h
    ./detail/strong_polymorphic_allocator.h
    ./detail/traits.h
    ./detail/utility.h)
    
set(eventual_inline
    ./eventual_methods.inl
    ./future.inl
    ./packaged_task.inl
    ./promise.inl
    ./shared_future.inl)
    
set(eventual_inline_detail
    ./detail/BasicFuture.inl
    ./detail/BasicPromise.inl
    ./detail/BasicTask.inl
    ./detail/CommonPromise.inl
    ./detail/CompositeState.inl
    ./detail/detail.inl
    ./detail/FutureFactory.inl
    ./detail/FutureHelper.inl
    ./detail/memory_resource.inl
    ./detail/polymorphic_allocator.inl
    ./detail/resource_adapter.inl
    ./detail/ResultBlock.inl
    ./detail/SimpleDelegate.inl
    ./detail/State.inl
    ./detail/strong_polymorphic_allocator.inl
    ./detail/utility.inl)
    
set(eventual_source
    ./eventual_methods.cpp
    ./future.cpp
    ./promise.cpp
    ./shared_future.cpp)
    
 set(eventual_source_detail
    ./detail/detail.cpp
    ./detail/memory_resource.cpp
    ./detail/SimpleDelegate.cpp
    ./detail/utility.cpp)

#dummy target
add_custom_target(eventual 
                  SOURCES 
                     ${eventual_headers} 
                     ${eventual_inline} 
                     ${eventual_source} 
                     ${eventual_headers_detail} 
                     ${eventual_inline_detail} 
                     ${eventual_source_detail})
                     
source_group("Header Files" FILES ${eventual_headers})
source_group("Header Files\\detail" FILES ${eventual_headers_detail})
source_group("Inline Files" FILES ${eventual_inline})
source_group("Inline Files\\detail" FILES ${eventual_inline_detail})
source_group("Source Files" FILES ${eventual_source})
source_group("Source Files\\detail" FILES ${eventual_source_detail})

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
            ${eventual_headers} 
            ${eventual_inline} 
            ${eventual_source}
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
