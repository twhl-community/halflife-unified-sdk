add_executable(mkmovie mkmovie.cpp)

target_link_libraries(mkmovie
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)

create_source_groups(mkmovie)
