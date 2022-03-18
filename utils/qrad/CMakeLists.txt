add_executable(qrad
	lightmap.cpp
	qrad.cpp
	qrad.h
	trace.cpp
	vismat.cpp)

target_include_directories(qrad
	PRIVATE
		../../src/engine)

target_link_libraries(qrad
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common)
