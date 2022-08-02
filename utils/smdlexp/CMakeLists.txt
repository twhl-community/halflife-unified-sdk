option(ENABLE_SMDLEXP "Enable the SMDLexp 3DS Max plugin" OFF)

if (NOT ENABLE_SMDLEXP)
	return()
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "smdlexp plugin can only be built as 64 bit")
endif()

# This library requires the 3DS Max SDK. You can download it here: https://www.autodesk.com/developer-network/platform-technologies/3ds-max

#Find the 3DS max dependencies. This is pretty ugly since it's hardcoded to the library's default install location.
set(MAX_SDK_DIR "C:/Program Files/Autodesk/3ds Max 2023 SDK/maxsdk")

set(MAX_INCLUDE_DIR "${MAX_SDK_DIR}/include")

if (NOT IS_DIRECTORY ${MAX_INCLUDE_DIR})
	message(FATAL_ERROR "Could not find 3DS Max include directory")
endif()

set(MAX_DEPENDENCIES)
foreach (lib_name maxutil.lib geom.lib mesh.lib core.lib)
	find_path(MAX_LIB ${lib_name} "${MAX_SDK_DIR}/lib/x64/Release")
	
	if (NOT MAX_LIB)
		message(FATAL_ERROR "Could not find 3DS Max library ${lib_name}")
	endif()
	
	list(APPEND MAX_DEPENDENCIES "${MAX_LIB}/${lib_name}")
endforeach()

add_library(smdlexp SHARED
	smdlexp.cpp
	smdlexp.def
	smdlexp.rc
	smedefs.h
	smexprc.h)

# This plugin requires the Unicode character set.
target_compile_definitions(smdlexp
	PRIVATE
		_UNICODE)

target_include_directories(smdlexp
	PRIVATE
		${MAX_INCLUDE_DIR})

target_link_libraries(smdlexp
	PRIVATE
		hl_sdk_utils_shared
		${MAX_DEPENDENCIES})

target_link_options(smdlexp
	PRIVATE
		"/DEF:${CMAKE_CURRENT_SOURCE_DIR}/smdlexp.def")

set_target_properties(smdlexp PROPERTIES SUFFIX ".dle")


create_source_groups(smdlexp)
