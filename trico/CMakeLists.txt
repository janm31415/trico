
set(HDRS
alloc.h
floating_point_stream_compression.h
transpose_aos_to_soa.h
trico_api.h
trico.h
)
	
set(SRCS
floating_point_stream_compression.c
transpose_aos_to_soa.c
trico.c
)

if (UNIX)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC")
endif (UNIX)

# general build definitions
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(TRICO_LIBRARY_TYPE STATIC)

if (${TRICO_SHARED} STREQUAL "yes")
add_definitions(-DTRICO_DLL_EXPORT)
set(TRICO_LIBRARY_TYPE SHARED)
endif (${TRICO_SHARED} STREQUAL "yes")

add_library(trico ${TRICO_LIBRARY_TYPE} ${HDRS} ${SRCS})

source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(trico
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    )	
    
target_link_libraries(trico
    PRIVATE	
    lz4
    )	
