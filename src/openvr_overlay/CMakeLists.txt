cmake_minimum_required (VERSION 3.8)

project(FreeScubaOverlay)
message("FreeScuba - Overlay")

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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source files
file(GLOB_RECURSE SOURCES_API ${CMAKE_SOURCE_DIR}/src/openvr_overlay "*.c" "*.h" "*.hpp" "*.cpp")
file(GLOB_RECURSE SOURCES_HEADERS ${CMAKE_SOURCE_DIR}/src/include "*.h" "*.hpp")

foreach(SOURCE IN ITEMS ${SOURCES_API})
    get_filename_component(SOURCE_PATH "${SOURCE}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP_PATH "${SOURCE_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SOURCE}")
endforeach()

set(IMGUI_FILES ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_dx11.cpp
              ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_win32.cpp)

add_definitions(-D_UNICODE)
add_executable(FreeScubaOverlay ${SOURCES_API} ${SOURCES_HEADERS} ${IMGUI_FILES})
GroupSourcesByFolder(FreeScubaOverlay)

target_include_directories(FreeScubaOverlay
	PUBLIC ${CMAKE_SOURCE_DIR}
	PUBLIC ${CMAKE_SOURCE_DIR}/src/openvr_overlay
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/openvr/headers
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui/backends)

target_link_libraries(FreeScubaOverlay
	PRIVATE dxgi.lib
	PRIVATE d3d11.lib
	PRIVATE d3dcompiler.lib
    	PRIVATE setupapi.lib
	PRIVATE ${CMAKE_SOURCE_DIR}/vendor/openvr/lib/win64/openvr_api.lib)

target_compile_definitions(FreeScubaOverlay
	PRIVATE NOMINMAX
)