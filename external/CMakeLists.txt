find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)