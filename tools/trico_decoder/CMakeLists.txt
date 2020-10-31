
set(HDRS
)
	
set(SRCS
main.c
)

# general build definitions
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)


if (${TRICO_SHARED} STREQUAL "yes")
add_definitions(-DTRICO_DLL_IMPORT)
endif (${TRICO_SHARED} STREQUAL "yes")

add_executable(trico_decoder ${HDRS} ${SRCS})

source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(trico_decoder
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    )	
    
target_link_libraries(trico_decoder
    PRIVATE	
    trico
    trico_io
    )	

if (UNIX)
  target_link_libraries(trico_decoder PRIVATE m )
endif (UNIX)