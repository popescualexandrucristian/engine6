function(add_shader TARGET SHADER)

	# Find glslc shader compiler.
	find_program(GLSL_LANG_VALIDATOR glslangValidator)
	
	# All shaders for a sample are found here.
	set(CURRENT_SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})

	set(CURRENT_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER}.spv)


	# Add a custom command to compile GLSL to SPIR-V.
	get_filename_component(current-output-dir ${CURRENT_OUTPUT_PATH} DIRECTORY)
	file(MAKE_DIRECTORY ${current-output-dir})
	
	if(ENABLE_DEBUG_SYMBOLS_IN_SHADERS)
		add_custom_command(
			OUTPUT ${CURRENT_OUTPUT_PATH}
			COMMAND ${GLSL_LANG_VALIDATOR} -g -V -o ${CURRENT_OUTPUT_PATH} ${CURRENT_SHADER_PATH}
			DEPENDS ${CURRENT_SHADER_PATH}
			IMPLICIT_DEPENDS CXX ${CURRENT_SHADER_PATH}
			VERBATIM)
	else()
		add_custom_command(
			OUTPUT ${CURRENT_OUTPUT_PATH}
			COMMAND ${GLSL_LANG_VALIDATOR} -g0 -V -o ${CURRENT_OUTPUT_PATH} ${CURRENT_SHADER_PATH}
			DEPENDS ${CURRENT_SHADER_PATH}
			IMPLICIT_DEPENDS CXX ${CURRENT_SHADER_PATH}
			VERBATIM)
	endif(ENABLE_DEBUG_SYMBOLS_IN_SHADERS)

	# Make sure our native build depends on this output.
	set_source_files_properties(${CURRENT_OUTPUT_PATH} PROPERTIES GENERATED TRUE)
	source_group(compiled_shaders FILES ${CURRENT_OUTPUT_PATH})
	target_sources(${TARGET} PRIVATE ${CURRENT_OUTPUT_PATH})
endfunction(add_shader)