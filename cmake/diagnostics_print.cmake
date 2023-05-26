###############################################
###          diagnostics_print              ###
###############################################
###
### Prints different diagnostics and infos
### about the specified target
###

function( diagnostics_print target_name )	
	message(STATUS "${target_name} settings:")
	message(STATUS "==============\n")

	message(STATUS "Source Files:")
	get_target_property(SOURCE_FILES ${target_name} SOURCES)
	if(SOURCE_FILES)
		message(STATUS "${SOURCE_FILES}")
	endif()
	
	message(STATUS "\nInclude Directories:")
	get_target_property(HEADER_DIR ${target_name} INCLUDE_DIRECTORIES)
	if(HEADER_DIR)
		message(STATUS "${HEADER_DIR}")
	endif()
	
	message(STATUS "\nLink Libraries:")
	get_target_property(LINKED_LIBS ${target_name} LINK_LIBRARIES)
	if(LINKED_LIBS)
		message(STATUS "${LINKED_LIBS}")
	endif()
		
	message(STATUS "\n")
endfunction()