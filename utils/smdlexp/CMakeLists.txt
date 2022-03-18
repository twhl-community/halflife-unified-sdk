add_executable(smdlexp 
	smdlexp.cpp
	smdlexp.def
	smdlexp.rc
	smedefs.h
	smexprc.h)

target_link_libraries(smdlexp
	PRIVATE
		hl_sdk_utils_shared)

target_link_options(smdlexp
	PRIVATE
		"/DEF:smdlexp.def")

set_target_properties(smdlexp PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)

create_source_groups(smdlexp)
