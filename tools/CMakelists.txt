
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_CURRENT_BINARY_DIR}/../bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")


add_subdirectory(stl_compress)
add_subdirectory(stl_decompress)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties (stl_compress PROPERTIES FOLDER tools)
set_target_properties (stl_decompress PROPERTIES FOLDER tools)

