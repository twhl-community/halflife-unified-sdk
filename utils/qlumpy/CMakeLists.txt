add_executable(qlumpy
	qlumpy.cpp
	qlumpy.h
	quakegrb.cpp)

target_link_libraries(qlumpy
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)

create_source_groups(qlumpy)
