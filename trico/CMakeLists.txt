
set(HDRS
alloc.h
floating_point_stream_compression.h
iostl.h
transpose_aos_to_soa.h
trico_api.h
trico.h
)
	
set(SRCS
floating_point_stream_compression.c
iostl.c
transpose_aos_to_soa.c
trico.c
)

if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
endif (WIN32)

# general build definitions
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(trico STATIC ${HDRS} ${SRCS})
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
