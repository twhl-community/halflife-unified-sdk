add_executable(light
	light.cpp
	light.h
	ltface.cpp
	trace.cpp)
	
target_link_libraries(light
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)
