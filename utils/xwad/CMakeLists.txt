add_executable(xwad xwad.cpp)

target_compile_definitions(xwad
	PRIVATE
		DOUBLEVEC_T)

target_link_libraries(xwad
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)
