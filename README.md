engine6
----

## Base for future projects built on Vulkan and examples on how to use https://github.com/popescualexandrucristian/acp_vulkan

* Simple example with data hardcoded in the shader.(when using cmake set EXAMPLE_NAME to Triange)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/triangle.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/triangle.jpg)
* Simple example with a quad displayed on the screen based on a vertex buffer and on an index buffer stored on the GPU.(when using cmake set EXAMPLE_NAME to Quad)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/quad_with_vertex_and_index.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/quad.jpg)
* Simple example with a textured quad displayed on the screen based on a vertex buffer and on an index buffer stored on the GPU.\
  The texture is a dds.(when using cmake set EXAMPLE_NAME to TexturedQuad)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/quad_with_vertex_index_and_texture.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/textured_quad.jpg)
* Simple example on how to integrate dear IMGUI with this libraries.\
  (Note: I am using a modified version that works with VMA and has some other improvements, see the readme there for details. The standard version also works)\
  (when using cmake set EXAMPLE_NAME to DearIMGUI)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/dear_imgui.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/imgui.jpg)
* Simple example that uses compute. (when using cmake set EXAMPLE_NAME to Compute)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/compute.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg)
* Simple example that uses tiny gltf to load a model. (when using cmake set EXAMPLE_NAME to GLTF)\
  https://raw.githubusercontent.com/popescualexandrucristian/engine6/main/src/renderer/vulkan/gltf.cpp \
  ![https://github.com/popescualexandrucristian/engine6/blob/main/docu/compute.jpg](https://github.com/popescualexandrucristian/engine6/blob/main/docu/tiny_gltf.jpg)


## Project generation
cmake -G "Visual Studio 17 2022" -DEXAMPLE_NAME=Compute

## Other cmake defines 

* USE_STATIC_CRT - Use the static crt when compileing on windows.
* ENABLE_ADDRESS_SENITIZER - Enable the address senitizer on the main project in debug and release with debug info. This does not work if the USE_STATIC_CRT option is set. Also on debug it only works within the VS ide or if the proper dlls are loaded to the path.
* ENABLE_DEBUG_CONSOLE - Enable the application's console.
* ENABLE_VULKAN_DEBUG_MARKERS - Enable markers that can name or tag objects and regions.
* ENABLE_VULKAN_VALIDATION_LAYERS - Enable Vulkan validation layers.
* ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER - Enable Vulkan validation syncronization layers.
* ENABLE_DEBUG_SYMBOLS_IN_SHADERS - Enable debug symbols in the generated shaders.
