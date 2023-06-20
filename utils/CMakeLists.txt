cmake_minimum_required(VERSION 3.21)

# Pull in the vcpkg repository from the mod SDK.
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

# Note: some of these projects will only work when built as 32 bit. Since they are largely the same as the original tools from the 1999 SDK they will need further changes to compile as 64 bit.

project(HalfLifeUtils
	VERSION 1.0
	DESCRIPTION "Half-Life SDK tools"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# Function to simplify generation of source groups.
function(create_source_groups target)
	get_target_property(TARGET_SOURCES hl_sdk_utils_common INTERFACE_SOURCES)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${TARGET_SOURCES})

	get_target_property(TARGET_SOURCES ${target} SOURCES)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${TARGET_SOURCES})
endfunction()

# Library to provide shared project properties.
add_library(hl_sdk_utils_shared INTERFACE)

target_compile_definitions(hl_sdk_utils_shared INTERFACE
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS
	$<$<CONFIG:DEBUG>:_DEBUG>)

target_include_directories(hl_sdk_utils_shared INTERFACE
	common)

target_link_libraries(hl_sdk_utils_shared INTERFACE
	Threads::Threads)

target_compile_options(hl_sdk_utils_shared INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<CXX_COMPILER_ID:MSVC>:/wd4244 /wd4305 /wd26451>)

# Library to provide utility code.
add_library(hl_sdk_utils_common INTERFACE)

# Add these as interface files. Because of DOUBLEVEC_T these may differ from project to project.
target_sources(hl_sdk_utils_common
	INTERFACE
		common/bspfile.cpp
		common/bspfile.h
		common/cmdlib.cpp
		common/cmdlib.h
		common/lbmlib.cpp
		common/lbmlib.h
		common/mathlib.cpp
		common/mathlib.h
		common/polylib.cpp
		common/polylib.h
		common/scriplib.cpp
		common/scriplib.h
		common/threads.cpp
		common/threads.h
		common/wadlib.cpp
		common/wadlib.h)

target_link_libraries(hl_sdk_utils_common INTERFACE
	hl_sdk_utils_shared)
	
add_subdirectory(bspinfo)
add_subdirectory(light)
add_subdirectory(makefont)
add_subdirectory(makels)
add_subdirectory(mdlviewer)
add_subdirectory(mkmovie)
add_subdirectory(qbsp2)
add_subdirectory(qcsg)
add_subdirectory(qlumpy)
add_subdirectory(qrad)
add_subdirectory(serverctrl)
add_subdirectory(sprgen)
add_subdirectory(studiomdl)
add_subdirectory(visx2)
add_subdirectory(xwad)

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT bspinfo)
