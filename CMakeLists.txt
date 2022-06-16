cmake_minimum_required(VERSION 3.21)

# See https://cmake.org/cmake/help/latest/policy/CMP0063.html
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

if (MSVC)
	set(CMAKE_SYSTEM_VERSION 10.0)
endif()

#
# Mod-specific configuration.
#

# Controls the name of the server library. You should leave this as its default unless you need a different name (e.g. conflict with another library name).
# You will need to manually update the Github Actions workflow configuration file (.github/worksflows/ci-cd.yml) to use this name.
set(HalfLife_SERVER_LIB_NAME server)

#
# End mod-specific configuration.
#

project(HalfLifeMod
	VERSION 1.0.0
	DESCRIPTION "Half-Life CMake Mod Template"
	LANGUAGES CXX)

# Disable the use of absolute paths in library paths even in development builds.
set(CMAKE_SKIP_BUILD_RPATH ON)

# Get the Half-Life directory and mod name from the install path
cmake_path(HAS_PARENT_PATH CMAKE_INSTALL_PREFIX HAS_HALFLIFE_DIRECTORY)
cmake_path(HAS_STEM CMAKE_INSTALL_PREFIX HAS_MOD_NAME)

if (NOT HAS_HALFLIFE_DIRECTORY OR NOT HAS_MOD_NAME)
	message(FATAL_ERROR "The install path must point to a mod directory")
endif()

cmake_path(GET CMAKE_INSTALL_PREFIX PARENT_PATH HALFLIFE_DIRECTORY)
cmake_path(GET CMAKE_INSTALL_PREFIX STEM MOD_NAME)

# All Half-Life specific commands are prefixed with HalfLife so they show up in the same group in the CMake GUI
set(HalfLife_HLDS_DIRECTORY "" CACHE PATH "Path to the Half-Life Dedicated Server directory. Must be specified to automatically set up dedicated server debugging settings and to copy libraries")

# Default to Release so this doesn't confuse people when they're making quick mods.
set(RELEASE_TYPE Alpha Beta Release)
set(HalfLife_RELEASE_TYPE "Release" CACHE STRING "What type of release you're building")
set_property(CACHE HalfLife_RELEASE_TYPE PROPERTY STRINGS ${RELEASE_TYPE})

list(FIND RELEASE_TYPE "${HalfLife_RELEASE_TYPE}" RELEASE_TYPE_INDEX)
if(RELEASE_TYPE_INDEX EQUAL -1)
	message(FATAL_ERROR "HalfLife_RELEASE_TYPE must be one of: ${RELEASE_TYPE}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Link statically with the runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Because we have 2 libraries that use the same symbols we have to default visibility to hidden so there are no collisions,
# and so the symbols don't merge and cause problems like server code calling the client version of a function.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Find dependencies
find_package(Threads REQUIRED)

option(SPDLOG_BUILD_TESTS "" OFF)
option(SPDLOG_BUILD_TESTS_HO "" OFF)
option(SPDLOG_ENABLE_PCH "" ON)
add_subdirectory(external/spdlog
	spdlog-binary-dir
	EXCLUDE_FROM_ALL)
	
option(JSON_BuildTests "" OFF)
option(JSON_MultipleHeaders "" ON)
add_subdirectory(external/json
	json-binary-dir
	EXCLUDE_FROM_ALL)

option(JSON_VALIDATOR_BUILD_TESTS "" OFF)
add_subdirectory(external/json-schema-validator
	json-validator-binary-dir
	EXCLUDE_FROM_ALL)

add_subdirectory(external/Angelscript/sdk/angelscript/projects/cmake
	Angelscript-binary-dir
	EXCLUDE_FROM_ALL)

# Turn off all non-essential stuff so it doesn't clog up the configuration log.
option(ALSOFT_UTILS "" OFF)
option(ALSOFT_EXAMPLES "" OFF)
option(ALSOFT_INSTALL_CONFIG "" OFF)
option(ALSOFT_INSTALL_HRTF_DATA "" OFF)
option(ALSOFT_INSTALL_AMBDEC_PRESETS "" OFF)
option(ALSOFT_INSTALL_EXAMPLES "" OFF)
add_subdirectory(external/openal-soft
	openal-soft-binary-dir
	EXCLUDE_FROM_ALL)

# Change the name to something that can't conflict with other things in the Half-Life directory.
set_target_properties(OpenAL PROPERTIES OUTPUT_NAME "openal-${MOD_NAME}")

option(LIBNYQUIST_BUILD_EXAMPLE "" OFF)
add_subdirectory(external/libnyquist
	libnyquist-binary-dir
	EXCLUDE_FROM_ALL)

# Enable multithreaded building to speed up building these a bit.
target_compile_options(nlohmann_json_schema_validator PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_options(angelscript PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_options(OpenAL PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

set_target_properties(
	spdlog
	nlohmann_json
	nlohmann_json_schema_validator
	angelscript
	build_version
	common
	OpenAL
	libnyquist
	PROPERTIES
		FOLDER "Dependencies")

# Generate DebugInfo.h with appropriate information.

# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE UNIFIED_SDK_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest tag name
execute_process(
    COMMAND git describe --abbrev=0 --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE UNIFIED_SDK_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# This can happen if there are no tags in the repository, so provide a suitable fallback.
if (UNIFIED_SDK_GIT_TAG STREQUAL "")
	set(UNIFIED_SDK_GIT_TAG "None (No tags found)")
endif()

# Get the latest commit hash
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE UNIFIED_SDK_GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
    COMMAND git status --porcelain=v1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE UNIFIED_SDK_GIT_STATUS_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Mark commit as dirty if there are uncommitted changes.
if (NOT UNIFIED_SDK_GIT_STATUS_RESULT STREQUAL "")
	set(UNIFIED_SDK_GIT_COMMIT_HASH "${UNIFIED_SDK_GIT_COMMIT_HASH}-dirty")
endif()

configure_file(
	src/game/shared/ProjectInfo.h.in
	${CMAKE_BINARY_DIR}/ProjectInfo.h
	@ONLY)

# Sets properties common to all projects
function(set_common_properties TARGET_NAME)
	set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
	
	# Enable use of Clang-Tidy in code analysis. See [repo_root]/.clang-tidy for the checks used in the project.
	set_target_properties(${TARGET_NAME} PROPERTIES VS_GLOBAL_EnableClangTidyCodeAnalysis true)
		
	target_compile_definitions(${TARGET_NAME} PRIVATE
		_CRT_SECURE_NO_WARNINGS
		$<$<CONFIG:DEBUG>:_DEBUG>
		CLIENT_WEAPONS
		# These are defined for OSX as well
		$<$<PLATFORM_ID:Linux, Darwin>:
			POSIX
			_POSIX
			LINUX
			_LINUX
			GNUC
		>
		$<$<PLATFORM_ID:Darwin>:
			OSX
			_OSX
		>
		UNIFIED_SDK_CONFIG="$<CONFIG>")
		
	target_link_libraries(${TARGET_NAME} PRIVATE
		angelscript
		spdlog::spdlog
		Threads::Threads
		${CMAKE_DL_LIBS})
	
	# TODO: should be fixed in Angelscript's CMake project
	target_include_directories(${TARGET_NAME} PRIVATE
		${CMAKE_BINARY_DIR}
		${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/angelscript/include
		${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/add_on)

	# TODO: review Linux compiler flags
	target_compile_options(${TARGET_NAME} PRIVATE
		# force 387 for FP math so the precision between win32 and linux and osx match
		# Note: the pentium-m arch setting is not used for AMD systems in the original makefile
		# Since the arch settings used are i686 this means including the setting ensures original settings are used,
		# but it could cause problems for AMD targets
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-fpermissive -fno-strict-aliasing -Wno-invalid-offsetof -gdwarf-2 -g2 -march=pentium-m -mfpmath=387>
			
		$<$<CXX_COMPILER_ID:MSVC>:
			/W3 /MP>
			
		# These are all from the original Makefile
		# They have not been tested with CMake since there is no Mac system available to test on, so this might not work
		$<$<PLATFORM_ID:Darwin>:
			-fvisibility=hidden -Qunused-arguments -mmacosx-version-min=10.5 -fasm-blocks -arch i386 -march=prescott -momit-leaf-frame-pointer -mtune=core2>)

	target_link_options(${TARGET_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-gdwarf-2 -g2>
		$<$<PLATFORM_ID:Linux>:
			-Wl,-Map,${TARGET_NAME}_map.txt>)
	
	target_sources(${TARGET_NAME}
		PRIVATE
			${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/game/UnifiedSDK.natvis
			${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/add_on/scriptstdstring/scriptstdstring.cpp
			${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/add_on/scriptstdstring/scriptstdstring.h)
endfunction()

function(add_json_dependencies target)
	target_link_libraries(${target}
		PRIVATE
			nlohmann_json_schema_validator)
endfunction()

add_subdirectory(src/game/client)
add_subdirectory(src/game/server)

# Add generated files now so it doesn't cause source_group to fail.
target_sources(client PRIVATE ${CMAKE_BINARY_DIR}/ProjectInfo.h)
target_sources(${HalfLife_SERVER_LIB_NAME} PRIVATE ${CMAKE_BINARY_DIR}/ProjectInfo.h)

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client)

if (MSVC)
	set(HL_EXECUTABLE "hl.exe")
	set(HLDS_EXECUTABLE "hlds.exe")
	
	set(HalfLife_DEVELOPER_LEVEL "0" CACHE STRING "When launching from Visual Studio, the developer level is set to this value")
	set_property(CACHE HalfLife_DEVELOPER_LEVEL PROPERTY STRINGS 0 1 2 3)
	
	option(HalfLife_ENABLE_CHEATS "When launching from Visual Studio, enable cheats as well" OFF)
	option(HalfLife_ENABLE_CONSOLE "When launching from Visual Studio, enable the console on startup" OFF)
	
	set(HalfLife_ADDITIONAL_COMMAND_ARGUMENTS "" CACHE STRING "When launching from Visual Studio, additional command line arguments to add")
	
	set(COMMAND_ARGUMENTS "-game ${MOD_NAME} +developer ${HalfLife_DEVELOPER_LEVEL}")
	
	if (HalfLife_ENABLE_CHEATS)
		set(COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS} +sv_cheats 1")
	endif()
	
	if (HalfLife_ENABLE_CONSOLE)
		set(COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS} -console")
	endif()
	
	set(COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS} ${HalfLife_ADDITIONAL_COMMAND_ARGUMENTS}")

	# Set debugging settings to work on the mod directory
	set_target_properties(client PROPERTIES
		VS_DEBUGGER_COMMAND "${HALFLIFE_DIRECTORY}/${HL_EXECUTABLE}"
		VS_DEBUGGER_COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS}"
		VS_DEBUGGER_WORKING_DIRECTORY "${HALFLIFE_DIRECTORY}")

	# If HalfLife_HLDS_DIRECTORY is not specified it will leave a non-functional "default" setting
	set_target_properties(${HalfLife_SERVER_LIB_NAME} PROPERTIES
		VS_DEBUGGER_COMMAND "${HalfLife_HLDS_DIRECTORY}/${HLDS_EXECUTABLE}"
		VS_DEBUGGER_COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS}"
		VS_DEBUGGER_WORKING_DIRECTORY "${HalfLife_HLDS_DIRECTORY}")
endif()
	
# Set install paths to the user-defined mod directory
install(TARGETS client
	RUNTIME DESTINATION cl_dlls
	LIBRARY DESTINATION cl_dlls
)

install(TARGETS ${HalfLife_SERVER_LIB_NAME}
	RUNTIME DESTINATION dlls
	LIBRARY DESTINATION dlls
)

configure_file(config/liblist.gam.in liblist.gam @ONLY)

install(FILES 
	config/network/delta.lst
	${CMAKE_BINARY_DIR}/liblist.gam
	DESTINATION .)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:client> DESTINATION cl_dlls OPTIONAL)
	install(FILES $<TARGET_PDB_FILE:${HalfLife_SERVER_LIB_NAME}> DESTINATION dlls OPTIONAL)
endif()

install(TARGETS OpenAL
	RUNTIME DESTINATION ..
	LIBRARY DESTINATION ..)

# Set up the script that will copy the mod libraries to the HLDS directory
if (NOT ${HalfLife_HLDS_DIRECTORY} STREQUAL "")
	set(hldsInstallScriptFile ${CMAKE_CURRENT_BINARY_DIR}/InstallToHLDS.cmake)
	configure_file(cmake/InstallToHLDS.cmake.in ${hldsInstallScriptFile} @ONLY)
	install(SCRIPT ${hldsInstallScriptFile})
endif()
	
