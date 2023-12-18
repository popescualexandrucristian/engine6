find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

add_library(acp_vulkan STATIC)
if(USE_STATIC_CRT AND MSVC)
target_compile_options(acp_vulkan PRIVATE /MT)
target_link_options(acp_vulkan PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
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
target_include_directories(acp_vulkan PUBLIC acp_vulkan)

if(USE_STATIC_CRT AND MSVC)
target_compile_options(acp_vulkan PRIVATE /MT)
target_link_options(acp_vulkan PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
endif()
if(ENABLE_ADDRESS_SENITIZER)
	target_compile_options(acp_vulkan PRIVATE "$<$<CONFIG:DEBUG>:/fsanitize=address>")
	target_compile_options(acp_vulkan PRIVATE "$<$<CONFIG:RelWithDebInfo>:/fsanitize=address>")
endif()