cmake_minimum_required (VERSION 3.8)

project(FreeScubaOverlay)
message("FreeScuba - Overlay")

set(CMAKE_CXX_STANDARD 14)

# Add source files
file(GLOB_RECURSE SOURCES_API RELATIVE ${CMAKE_SOURCE_DIR}/src/openvr_overlay "*.c" "*.h" "*.hpp" "*.cpp")
file(GLOB_RECURSE SOURCES_HEADERS RELATIVE ${CMAKE_SOURCE_DIR}/src/include "*.h" "*.hpp")

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

add_executable(FreeScubaOverlay ${SOURCES_API} ${SOURCES_HEADERS} ${IMGUI_FILES})

target_include_directories(FreeScubaOverlay
	PUBLIC ${CMAKE_SOURCE_DIR}
	PUBLIC ${CMAKE_SOURCE_DIR}/src/openvr_overlay
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/openvr/headers
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui
	PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui/backends)

target_link_libraries(FreeScubaOverlay
	PRIVATE dxgi.lib
	PRIVATE d3d11.lib
	PRIVATE d3dcompiler.lib
	PRIVATE ${CMAKE_SOURCE_DIR}/vendor/openvr/lib/win64/openvr_api.lib)