find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

add_library(acp_vulkan STATIC)
if(USE_STATIC_CRT AND MSVC)
	target_compile_options(acp_vulkan PRIVATE /MT)
	target_link_options(acp_vulkan PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
elseif(ENABLE_ADDRESS_SENITIZER)
	target_compile_options(acp_vulkan PRIVATE "$<$<CONFIG:DEBUG>:/fsanitize=address>")
	target_compile_options(acp_vulkan PRIVATE "$<$<CONFIG:RelWithDebInfo>:/fsanitize=address>")
	target_link_options(acp_vulkan PRIVATE /INCREMENTAL:NO)
endif()

set_property(TARGET acp_vulkan PROPERTY FOLDER "libraries")
target_sources(acp_vulkan PRIVATE 
    acp_vulkan/acp_dds_vulkan.h
    acp_vulkan/acp_dds_vulkan.cpp
	acp_vulkan/acp_program_vulkan.h
    acp_vulkan/acp_program_vulkan.cpp
	acp_vulkan/acp_context/acp_vulkan_context.h
    acp_vulkan/acp_context/acp_vulkan_context.cpp
	acp_vulkan/acp_context/acp_vulkan_context_utils.h
    acp_vulkan/acp_context/acp_vulkan_context_utils.cpp
	acp_vulkan/acp_context/acp_vulkan_context_swapchain.h
    acp_vulkan/acp_context/acp_vulkan_context_swapchain.cpp
    )
if(MSVC)
target_sources(acp_vulkan PRIVATE 
	acp_vulkan/acp_context/win32/acp_vulkan_context_win32.cpp
	)
endif()
target_include_directories(acp_vulkan PUBLIC acp_vulkan)

target_compile_definitions(acp_vulkan PRIVATE ${COMPILATION_DEFINES})

add_library(imgui STATIC)
if(USE_STATIC_CRT AND MSVC)
	target_compile_options(imgui PRIVATE /MT)
	target_link_options(imgui PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
elseif(ENABLE_ADDRESS_SENITIZER)
	target_compile_options(imgui PRIVATE "$<$<CONFIG:DEBUG>:/fsanitize=address>")
	target_compile_options(imgui PRIVATE "$<$<CONFIG:RelWithDebInfo>:/fsanitize=address>")
	target_link_options(imgui PRIVATE /INCREMENTAL:NO)
endif()

set_property(TARGET imgui PROPERTY FOLDER "libraries")

target_sources(imgui PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp

    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp

    imgui/backends/imgui_impl_vulkan.cpp
	
	imgui/imgui_demo.cpp
    )
	
if(MSVC)
target_sources(imgui PRIVATE 
		imgui/backends/imgui_impl_win32.cpp
	)
endif()
target_include_directories(imgui PUBLIC imgui)

target_compile_definitions(imgui PRIVATE ${COMPILATION_DEFINES})