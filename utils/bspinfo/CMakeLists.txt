add_executable(bspinfo bspinfo.cpp)

target_link_libraries(bspinfo
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)

create_source_groups(bspinfo)
