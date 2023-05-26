###############################################
###          build_source_group             ###
###############################################
###
### Builds a source group from a set of files
### for nicer display in IDEs
###

function( build_source_group )
	file(GLOB_RECURSE files ${CMAKE_CURRENT_SOURCE_DIR}/*.* )
	list(REMOVE_ITEM files "CMakeLists.txt")
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "[src]" FILES ${files})
endfunction()