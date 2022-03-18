add_executable(makefont makefont.cpp)

target_include_directories(makefont
	PRIVATE
		../../src/common)

# TODO: replace winmm with std::chrono
target_link_libraries(makefont
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common
		winmm.lib)
