foreach(_example atoi awkemu clang concurrent cppscan format gotocallret
		mailbox params rlscan statechart pullscan)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_example}.cpp"
		DEPENDS ${_example}.rl
		COMMAND ragel
		ARGS -G2 -o "${CMAKE_CURRENT_BINARY_DIR}/${_example}.cpp"
			"${CMAKE_CURRENT_LIST_DIR}/${_example}.rl"
		WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
	add_executable(${_example} "${CMAKE_CURRENT_BINARY_DIR}/${_example}.cpp")
endforeach()
