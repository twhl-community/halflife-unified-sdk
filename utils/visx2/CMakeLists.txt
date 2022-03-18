add_executable(vis
	flow.cpp
	soundpvs.cpp
	vis.cpp
	vis.h)

target_include_directories(vis
	PRIVATE
		../../src/engine)

target_link_libraries(vis
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)
