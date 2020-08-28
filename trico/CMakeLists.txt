
set(HDRS
alloc.h
floating_point_stream_compression.h
intrinsics.h
iostl.h
transpose_aos_to_soa.h
trico_api.h
trico.h
)
	
set(SRCS
floating_point_stream_compression.cpp
iostl.cpp
transpose_aos_to_soa.cpp
trico.cpp
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

add_library(trico SHARED ${HDRS} ${SRCS})
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
