add_executable(sprgen
	sprgen.cpp
	spritegn.h)

target_link_libraries(sprgen
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)
