
set(HDRS
fps_compression.h
int_compression.h
test_assert.h
timer.h
trico_compression.h
    )
	
set(SRCS
fps_compression.cpp
int_compression.cpp
test_assert.cpp
test.cpp
trico_compression.cpp
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

add_definitions(-DMEMORY_LEAK_TRACKING)

if (${TRICO_SHARED} STREQUAL "yes")
add_definitions(-DTRICO_DLL_IMPORT)
endif (${TRICO_SHARED} STREQUAL "yes")

add_executable(trico.tests ${HDRS} ${SRCS})
source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})

target_include_directories(trico.tests
    PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../
    )
	
target_link_libraries(trico.tests
    PRIVATE
    trico
    zlib
    lz4
    )	
  
if (WIN32)
add_custom_command(TARGET trico.tests POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" ${CMAKE_CURRENT_BINARY_DIR}/data)
endif (WIN32)    

if (UNIX)
add_custom_command(TARGET trico.tests POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:trico.tests>/data")
endif (UNIX)