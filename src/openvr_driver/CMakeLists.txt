cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This is your project name
set(TARGET_NAME freescuba)

# This is the name that the binary will be called (driver_freescuba)
set(DRIVER_NAME "driver_${TARGET_NAME}")

project(FreeScubaDriver)
message("FreeScuba - Driver")

function(GroupSourcesByFolder target)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()


set(OPENVR_LIB_DIR ${CMAKE_SOURCE_DIR}/vendor/openvr)

set(CMAKE_CXX_STANDARD 20)

# If not set, determine the platform architecture
if (NOT PLATFORM)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(PLATFORM 64)
    else ()
        set(PLATFORM 32)
    endif ()
endif ()
message(STATUS "Compilation set for ${PLATFORM}bits architectures.")

# OpenVR compatibility checking
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX -DPOSIX)
    set(ARCH_TARGET linux64)

    if (${PLATFORM} MATCHES 32)
        message(WARNING "OpenVR x86 binaries not provided on GNU/Linux.")
    endif ()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH 0)
    add_definitions(-DOSX -DPOSIX)
    set(ARCH_TARGET osx32)

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-D_WIN32)
    set(ARCH_TARGET win${PLATFORM})

    # Binaries path for thirdparties are not generics so we try to guess their suffixes.
    set(WINDOWS_PATH_SUFFIXES win${PLATFORM} Win${PLATFORM} x${PLATFORM})
endif ()

find_library(OPENVR_LIBRARIES
        NAMES
        openvr_api
        PATHS
        ${OPENVR_LIB_DIR}/bin
        ${OPENVR_LIB_DIR}/lib
        PATH_SUFFIXES
        osx${PLATFORM}
        linux${PLATFORM}
        win${PLATFORM}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
        )

# Add source files
file(GLOB_RECURSE SOURCES_API ${CMAKE_SOURCE_DIR}/src/openvr_driver "*.c" "*.h" "*.hpp" "*.cpp")
file(GLOB_RECURSE SOURCES_HEADERS ${CMAKE_SOURCE_DIR}/src/openvr_driver "*.h" "*.hpp")

foreach(SOURCE IN ITEMS ${SOURCES_API})
    get_filename_component(SOURCE_PATH "${SOURCE}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP_PATH "${SOURCE_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SOURCE}")
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/${DRIVER_NAME}/bin/${ARCH_TARGET}>)

add_definitions(-D_UNICODE)
add_library(FreeScubaDriver SHARED ${SOURCES_API} ${SOURCES_HEADERS})
GroupSourcesByFolder(FreeScubaDriver)

target_include_directories(FreeScubaDriver
	PRIVATE ${CMAKE_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/openvr_driver
	PRIVATE ${OPENVR_LIB_DIR}
	PRIVATE ${CMAKE_SOURCE_DIR}/vendor/openvr/headers
	PRIVATE ${CMAKE_SOURCE_DIR}/vendor/minhook/include)

target_link_libraries(FreeScubaDriver
	PRIVATE d3d11.lib
	PRIVATE dxgi.lib
	PRIVATE d3dcompiler.lib
	PRIVATE ${OPENVR_LIBRARIES}
	PRIVATE minhook)

set_target_properties(FreeScubaDriver PROPERTIES OUTPUT_NAME ${DRIVER_NAME})

# Copy driver assets to output folder
add_custom_command(
    TARGET FreeScubaDriver
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/${DRIVER_NAME}
    ${CMAKE_BINARY_DIR}/${DRIVER_NAME}
)