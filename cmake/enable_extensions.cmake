###############################################
###          enable_extensions              ###
###############################################
###
### Enable proprietary DRAMSys extensions
### during build generation
###

function( dramsys_enable_extensions )
	file(GLOB sub_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${DRAMSYS_EXTENSIONS_DIR}/*)

	message(STATUS "Enabling DRAMSys extensions:")
	message(STATUS "============================")

	FOREACH(sub_dir ${sub_dirs})
		IF(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${sub_dir}")
			add_subdirectory(${sub_dir})
		ENDIF()
	ENDFOREACH()
	
	message(STATUS "")
	message(STATUS "")
endfunction()