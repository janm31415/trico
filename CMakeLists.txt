cmake_minimum_required(VERSION 3.10.0)
project (trico)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TRICO_SHARED "no" CACHE STRING  "Choose yes for a shared dll, or no for a static library.")
set_property(CACHE TRICO_SHARED PROPERTY STRINGS yes no)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(lz4)
add_subdirectory(rply)
add_subdirectory(tools)
add_subdirectory(trico)
add_subdirectory(trico.tests)


