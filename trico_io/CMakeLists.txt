
set(HDRS
ioply.h
iostl.h
trico_io_api.h
)
	
set(SRCS
ioply.c
iostl.c
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

set(TRICO_IO_LIBRARY_TYPE STATIC)

if (${TRICO_SHARED} STREQUAL "yes")
add_definitions(-DTRICO_DLL_EXPORT)
set(TRICO_IO_LIBRARY_TYPE SHARED)
endif (${TRICO_SHARED} STREQUAL "yes")

add_library(trico_io ${TRICO_IO_LIBRARY_TYPE} ${HDRS} ${SRCS})

source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(trico_io
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    )	
    
target_link_libraries(trico_io
    PRIVATE	
    rply
    )	
