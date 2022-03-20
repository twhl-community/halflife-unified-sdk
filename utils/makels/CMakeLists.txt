add_executable(makels makels.cpp)

target_link_libraries(makels
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)

create_source_groups(makels)
