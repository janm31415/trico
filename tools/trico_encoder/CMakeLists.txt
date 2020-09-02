
set(HDRS
)
	
set(SRCS
main.c
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


if (${TRICO_SHARED} STREQUAL "yes")
add_definitions(-DTRICO_DLL_IMPORT)
endif (${TRICO_SHARED} STREQUAL "yes")

add_executable(trico_encoder ${HDRS} ${SRCS})

source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(trico_encoder
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    )	
    
target_link_libraries(trico_encoder
    PRIVATE	
    trico
    trico_io
    )	
