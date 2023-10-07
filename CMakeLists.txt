cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
			COMMAND head -n 1
			COMMAND cut -c9-
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME playdate-opusfile-test)
set(PLAYDATE_GAME_DEVICE playdate-opusfile-test_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

if (TOOLCHAIN STREQUAL "armgcc")
	# Hack: avoid linker errors (https://devforum.play.date/t/c-api-converting-string-to-float/10097/7)
	set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs" CACHE INTERNAL "")

	add_executable(${PLAYDATE_GAME_DEVICE} src/main.c)
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED src/main.c )
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)

set(OP_DISABLE_HTTP ON)
set(OP_DISABLE_DOCS ON)
set(OP_DISABLE_EXAMPLES ON)
add_subdirectory(lib/opus)
add_subdirectory(lib/ogg)
add_subdirectory(lib/opusfile)

if (TOOLCHAIN STREQUAL "armgcc")
	target_link_libraries(${PLAYDATE_GAME_DEVICE} opusfile)
else()
	target_link_libraries(${PLAYDATE_GAME_NAME} opusfile)
endif()