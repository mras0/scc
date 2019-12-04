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

macro(SCC n)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${n}.com"
        COMMAND $<TARGET_FILE:scc> "${CMAKE_CURRENT_SOURCE_DIR}/${n}.c" ./${n}.com
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${n}.c"
    )

    add_custom_target(${n}_com ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${n}.com")
endmacro()

SCC(scc)
SCC(sas)

add_subdirectory(test)
