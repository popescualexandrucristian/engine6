cmake_minimum_required (VERSION 3.20.0)
project (engine6 VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)



option(USE_STATIC_CRT "Use the static crt when compileing on windows" OFF)
option(ENABLE_ADDRESS_SENITIZER "Enable the address senitizer on the main project in debug and release with debug info" OFF)

option(ENABLE_DEBUG_CONSOLE "Enable the application's console" OFF)

option(ENABLE_VULKAN_DEBUG_MARKERS "Enable markers that can name or tag objects and regions" OFF)
option(ENABLE_VULKAN_VALIDATION_LAYERS "Enable Vulkan validation layers" OFF)
option(ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER "Enable Vulkan validation syncronization layers" OFF)
option(ENABLE_DEBUG_SYMBOLS_IN_SHADERS "Enable debug symbols in the generated shaders" OFF)
set(EXAMPLE_NAME "DearIMGUI" CACHE STRING "Target example")
set_property(CACHE EXAMPLE_NAME PROPERTY STRINGS Triange Quad TexturedQuad DearIMGUI Compute)

set(PROJECT_NAME engine6)

configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.h.in"
    "${PROJECT_BINARY_DIR}/generated/version.h"
)
include_directories("${PROJECT_BINARY_DIR}/generated")

set(EXTERNAL_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external")

find_package(Vulkan)
include_directories(${Vulkan_INCLUDE_DIRS})

set(MAIN_RENDERER
		src/renderer/vulkan/triangle.h
		src/renderer/vulkan/triangle.cpp
		
		src/renderer/vulkan/quad_with_vertex_and_index.h
		src/renderer/vulkan/quad_with_vertex_and_index.cpp
		
		src/renderer/vulkan/quad_with_vertex_index_and_texture.h
		src/renderer/vulkan/quad_with_vertex_index_and_texture.cpp

		src/renderer/vulkan/dear_imgui.h
		src/renderer/vulkan/dear_imgui.cpp
		
		src/renderer/vulkan/compute.h
		src/renderer/vulkan/compute.cpp
	)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/vulkan")
source_group(renderer FILES ${MAIN_RENDERER})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
if(WIN32)
	set(MAIN_SRC
		src/win32/main.cpp
	)
else()
	message(FATAL_ERROR "Invalid platform")
endif()

source_group(main FILES ${MAIN_SRC})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/shaders_build)

#shaders
file(GLOB ALL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/vulkan/shaders/*.*)

#external
set(LIBS
	${Vulkan_LIBRARIES}
	acp_vulkan
	vma
	imgui
	nlohmann_json
	tiny_gltf
)

#main
if(WIN32)
	add_executable(${CMAKE_PROJECT_NAME} WIN32 
		${MAIN_SRC}
		${MAIN_RENDERER}
		${ALL_SHADERS}
	)
	
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})
	
else()
	message(FATAL_ERROR "Invalid platform")
endif()

if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4316 /wd4324 /wd26495 /wd26812 /wd28023" )

if(ENABLE_VULKAN_VALIDATION_LAYERS)
set (ENABLE_VULKAN_VALIDATION_LAYERS_DEFINE
	ENABLE_VULKAN_VALIDATION_LAYERS=1
)
else()
set (ENABLE_VULKAN_VALIDATION_LAYERS_DEFINE
)
endif()

if(ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER)
set (ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER_DEFINE
	ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER=1
)
else()
set (ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER_DEFINE
)
endif()

if(ENABLE_DEBUG_CONSOLE)
set (ENABLE_DEBUG_CONSOLE_DEFINE
	ENABLE_DEBUG_CONSOLE=1
)
else()
set (ENABLE_DEBUG_CONSOLE_DEFINE
)
endif()

if(ENABLE_VULKAN_DEBUG_MARKERS)
set (ENABLE_VULKAN_DEBUG_MARKERS_DEFINE
	ENABLE_VULKAN_DEBUG_MARKERS=1
)
else()
set (ENABLE_VULKAN_DEBUG_MARKERS_DEFINE
)
endif()

if(EXAMPLE_NAME STREQUAL Triange)
set (EXAMPLE_DEFINE
	TRIANGLE_EXAMPLE=1
)
elseif(EXAMPLE_NAME STREQUAL Quad)
set (EXAMPLE_DEFINE
	QUAD_EXAMPLE=1
)
elseif(EXAMPLE_NAME STREQUAL TexturedQuad)
set (EXAMPLE_DEFINE
	QUAD_WITH_TEXTURE_EXAMPLE=1
)
elseif(EXAMPLE_NAME STREQUAL DearIMGUI)
set (EXAMPLE_DEFINE
	DEAR_IMGUI_EXAMPLE=1
)
elseif(EXAMPLE_NAME STREQUAL Compute)
set (EXAMPLE_DEFINE
	COMPUTE_EXAMPLE=1
)
else()
set (EXAMPLE_DEFINE
)
endif()

set (COMPILATION_DEFINES
	_CRT_SECURE_NO_WARNINGS=1 
	WIN32_LEAN_AND_MEAN=1
	#_SECURE_SCL=0
	#_HAS_ITERATOR_DEBUGGING=0
	${ENABLE_DEBUG_CONSOLE_DEFINE}
	${ENABLE_VULKAN_VALIDATION_LAYERS_DEFINE}
	${ENABLE_VULKAN_DEBUG_MARKERS_DEFINE}
	${ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER_DEFINE}
	${EXAMPLE_DEFINE}
)

#external
add_subdirectory (${EXTERNAL_LIB_PATH})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${COMPILATION_DEFINES})

target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBS})

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if(USE_STATIC_CRT AND MSVC)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /MT)
	target_link_options(${CMAKE_PROJECT_NAME} PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
elseif(ENABLE_ADDRESS_SENITIZER)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/fsanitize=address>")
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "$<$<CONFIG:RelWithDebInfo>:/fsanitize=address>")
	target_link_options(${CMAKE_PROJECT_NAME} PRIVATE /INCREMENTAL:NO)
endif()

#build shaders
source_group(renderer/shaders FILES ${ALL_SHADERS})
foreach(SHADER ${ALL_SHADERS})
		get_filename_component(p ${SHADER} NAME)
		add_shader(${CMAKE_PROJECT_NAME} ${p})
endforeach(SHADER)

file(COPY ${PROJECT_SOURCE_DIR}/textures DESTINATION ${CMAKE_SOURCE_DIR}/bin)
file(GLOB FONT_FILES
  "${PROJECT_SOURCE_DIR}/external/imgui/misc/fonts/*.ttf"
)
file(COPY ${FONT_FILES} DESTINATION ${CMAKE_SOURCE_DIR}/bin/fonts)