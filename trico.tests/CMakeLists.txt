
set(HDRS
fps_compression.h
test_assert.h
    )
	
set(SRCS
fps_compression.cpp
test_assert.cpp
test.cpp
)


if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")

if (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /arch:AVX2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /arch:AVX2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:AVX2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
endif (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")

endif (WIN32)

if (UNIX)
if (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -mavx2 -mfma")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -mavx2 -mfma")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mavx2 -mfma")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx2 -mfma")
endif (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")
if (${TRICO_VECTOR_INTRINSICS} STREQUAL "sse2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -msse2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -msse2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -msse2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -msse2")
endif (${TRICO_VECTOR_INTRINSICS} STREQUAL "sse2")
endif (UNIX)

# general build definitions
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")
add_definitions(-D_AVX2)
endif (${TRICO_VECTOR_INTRINSICS} STREQUAL "avx2")

if (${TRICO_VECTOR_INTRINSICS} STREQUAL "sse2")
add_definitions(-D_SSE2)
endif (${TRICO_VECTOR_INTRINSICS} STREQUAL "sse2")

add_definitions(-DMEMORY_LEAK_TRACKING)


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
    )	
    
add_custom_command(TARGET trico.tests POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" ${CMAKE_CURRENT_BINARY_DIR}/data)
    