cmake_minimum_required (VERSION 3.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY   		${CMAKE_BINARY_DIR}/artifacts)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY   		${CMAKE_BINARY_DIR}/artifacts)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY   		${CMAKE_BINARY_DIR}/artifacts)

# Function because lazy
function(adjust_bin_paths lib)
  set_target_properties(${lib}
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY 			${CMAKE_BINARY_DIR}/artifacts
		LIBRARY_OUTPUT_DIRECTORY 			${CMAKE_BINARY_DIR}/artifacts
		ARCHIVE_OUTPUT_DIRECTORY 			${CMAKE_BINARY_DIR}/artifacts
	)
endfunction()

project("freescuba")

# Include project
add_subdirectory ("src")

# Libs
add_subdirectory ("vendor")