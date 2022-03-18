add_library(procinfo STATIC)

target_sources(procinfo
	PRIVATE
		procinfo.cpp
	INTERFACE
		procinfo.h)

target_link_libraries(procinfo
	INTERFACE
		hl_sdk_utils_shared)
