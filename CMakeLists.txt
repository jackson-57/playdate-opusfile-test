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
	foreach(LOOP_TARGET opus ogg opusfile)
		#	target_compile_definitions(${LOOP_TARGET} PUBLIC TARGET_PLAYDATE=1)
		target_compile_options(${LOOP_TARGET} PUBLIC -Wall -Wno-unknown-pragmas -Wdouble-promotion)
		target_compile_options(${LOOP_TARGET} PUBLIC $<$<CONFIG:DEBUG>:-O2>)
		target_compile_options(${LOOP_TARGET} PUBLIC $<$<CONFIG:RELEASE>:-O2>)

		target_compile_options(${LOOP_TARGET} PUBLIC ${MCFLAGS})
		target_compile_options(${LOOP_TARGET} PUBLIC -falign-functions=16 -fomit-frame-pointer)
		target_compile_options(${LOOP_TARGET} PUBLIC -gdwarf-2)
		target_compile_options(${LOOP_TARGET} PUBLIC -fverbose-asm)
		target_compile_options(${LOOP_TARGET} PUBLIC -ffunction-sections -fdata-sections)
		target_compile_options(${LOOP_TARGET} PUBLIC -mword-relocations -fno-common)

		target_compile_options(${LOOP_TARGET} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
	endforeach()

	target_link_libraries(${PLAYDATE_GAME_DEVICE} opusfile)
else()
	target_link_libraries(${PLAYDATE_GAME_NAME} opusfile)
endif()