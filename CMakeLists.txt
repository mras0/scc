cmake_minimum_required(VERSION 3.7)
project(scc C)

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

	# warning C4715: 'XXX': not all control paths return a value
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi /Zo /sdl /W4 /wd4715")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Wall -Wextra -pedantic -std=c99 -Wno-return-type")
endif()

add_executable(scc scc.c)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT scc)
endif()

add_custom_command(
    OUTPUT scc.com
    COMMAND $<TARGET_FILE:scc> scc.c
    COMMAND ${CMAKE_COMMAND} -E copy scc.com "${CMAKE_CURRENT_BINARY_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(scc_com DEPENDS scc.com)

add_subdirectory(test)
