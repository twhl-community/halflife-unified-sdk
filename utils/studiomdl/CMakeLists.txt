add_executable(studiomdl
	bmpread.cpp
	studiomdl.cpp
	studiomdl.h
	tristrip.cpp
	write.cpp)

target_include_directories(studiomdl
	PRIVATE
		../../src/common
		../../src/engine
		../../src/game/server
		../../src/public)

target_link_libraries(studiomdl
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)

create_source_groups(studiomdl)
