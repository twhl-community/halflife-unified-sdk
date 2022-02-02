cmake_minimum_required(VERSION 3.21)

if (MSVC)
	set(CMAKE_SYSTEM_VERSION 10.0)
endif()

project(HalfLifeMod
	VERSION 1.0
	DESCRIPTION "Half-Life CMake Mod Template"
	LANGUAGES CXX)

if (UNIX)
	# Mods are always 32 bit, so force this to use the i386 architecture
	set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
endif()

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

# Enable multithreaded building to speed up building these a bit.
target_compile_options(nlohmann_json_schema_validator PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_options(angelscript PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

set_target_properties(
	spdlog
	nlohmann_json
	nlohmann_json_schema_validator
	angelscript
	PROPERTIES
		FOLDER "Dependencies")

if (UNIX)
	function(set_32bit_compilation TARGET_NAME)
		target_compile_options(${TARGET_NAME} PRIVATE -m32)
		target_link_options(${TARGET_NAME} PRIVATE -m32)
	endfunction()

	set_32bit_compilation(spdlog)
	#set_32bit_compilation(nlohmann_json) # Don't set it for this one, it's an INTERFACE library (header-only)
	set_32bit_compilation(nlohmann_json_schema_validator)
	set_32bit_compilation(angelscript)
endif()

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
		>)
		
	target_link_libraries(${TARGET_NAME} PRIVATE
		angelscript
		spdlog::spdlog
		Threads::Threads
		${CMAKE_DL_LIBS})
	
	# TODO: should be fixed in Angelscript's CMake project
	target_include_directories(${TARGET_NAME} PRIVATE
		${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/angelscript/include
		${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external/Angelscript/sdk/add_on)

	# TODO: review Linux compiler flags
	target_compile_options(${TARGET_NAME} PRIVATE
		# force 387 for FP math so the precision between win32 and linux and osx match
		# Note: the pentium-m arch setting is not used for AMD systems in the original makefile
		# Since the arch settings used are i686 this means including the setting ensures original settings are used,
		# but it could cause problems for AMD targets
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-m32 -fpermissive -fno-strict-aliasing -Wno-invalid-offsetof -gdwarf-2 -g2 -march=pentium-m -mfpmath=387>
			
		$<$<CXX_COMPILER_ID:MSVC>:
			/W3 /MP>
			
		# These are all from the original Makefile
		# They have not been tested with CMake since there is no Mac system available to test on, so this might not work
		$<$<PLATFORM_ID:Darwin>:
			-fvisibility=hidden -Qunused-arguments -mmacosx-version-min=10.5 -fasm-blocks -arch i386 -march=prescott -momit-leaf-frame-pointer -mtune=core2>)

	target_link_options(${TARGET_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-m32 -gdwarf-2 -g2>
		$<$<PLATFORM_ID:Linux>:
			-Wl,-Map,${TARGET_NAME}_map.txt>)
	
	target_sources(${TARGET_NAME}
		PRIVATE
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
	set_target_properties(hl PROPERTIES
		VS_DEBUGGER_COMMAND "${HalfLife_HLDS_DIRECTORY}/${HLDS_EXECUTABLE}"
		VS_DEBUGGER_COMMAND_ARGUMENTS "${COMMAND_ARGUMENTS}"
		VS_DEBUGGER_WORKING_DIRECTORY "${HalfLife_HLDS_DIRECTORY}")
endif()
	
# Set install paths to the user-defined mod directory
install(TARGETS client
	RUNTIME DESTINATION cl_dlls
	LIBRARY DESTINATION cl_dlls
)

install(TARGETS hl
	RUNTIME DESTINATION dlls
	LIBRARY DESTINATION dlls
)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:client> DESTINATION cl_dlls OPTIONAL)
	install(FILES $<TARGET_PDB_FILE:hl> DESTINATION dlls OPTIONAL)
endif()

# Set up the script that will copy the mod libraries to the HLDS directory
if (NOT ${HalfLife_HLDS_DIRECTORY} STREQUAL "")
	set(hldsInstallScriptFile ${CMAKE_CURRENT_BINARY_DIR}/InstallToHLDS.cmake)
	configure_file(cmake/InstallToHLDS.cmake.in ${hldsInstallScriptFile} @ONLY)
	install(SCRIPT ${hldsInstallScriptFile})
endif()
	
