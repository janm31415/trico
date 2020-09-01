
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_CURRENT_BINARY_DIR}/../bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")


add_subdirectory(trico_encoder)
add_subdirectory(trico_decoder)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties (trico_encoder PROPERTIES FOLDER tools)
set_target_properties (trico_decoder PROPERTIES FOLDER tools)

