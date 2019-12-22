cmake_minimum_required(VERSION 3.7)
project(scc C)

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

	# warning C4715: 'X': not all control paths return a value
    # warning C4244: 'X': conversion from 'T' to 'U', possible loss of data
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi /Zo /sdl /W4 /wd4715 /wd4244")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Wall -Wextra -pedantic -std=c99 -Wno-return-type")
endif()

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT scc)
endif()

macro(SCC n)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${n}.com"
        COMMAND $<TARGET_FILE:scc> "${CMAKE_CURRENT_SOURCE_DIR}/${n}.c" ./${n}.com
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${n}.c" scc
    )

    add_custom_target(${n}_com ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${n}.com")

    add_executable(${n} ${n}.c) # Make sure we can build on host as well
endmacro()

SCC(scc)
SCC(scpp)
SCC(sas)

# Simulator
add_executable(sim86 sim86.c lib.h)
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sim86.com"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/lib.h" .
        COMMAND $<TARGET_FILE:scpp> "${CMAKE_CURRENT_SOURCE_DIR}/sim86.c" ./sim86.i
        COMMAND $<TARGET_FILE:scc>  sim86.i sim86.com
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sim86.c" "${CMAKE_CURRENT_SOURCE_DIR}/lib.h" scc scpp
    )
add_custom_target(sim86_com ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sim86.com")

add_subdirectory(test)

set(MAKE_DISKS FALSE CACHE BOOL "Build disk images? (Downloads SASM)")
if (MAKE_DISKS)
    include(ExternalProject)
    ExternalProject_Add(SASM
        GIT_REPOSITORY https://github.com/mras0/sasm.git
        GIT_TAG v1.1
        INSTALL_COMMAND ""
        )
    ExternalProject_Get_property(SASM SOURCE_DIR)
    ExternalProject_Get_property(SASM BINARY_DIR)
    set(DISKTOOL "${BINARY_DIR}/disktool")
    set(BOOTSECT "${BINARY_DIR}/boot.bin")
    macro(MAKEDISK NAME SIZE)
        set(DISK_IMG "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.img")
        message(STATUS "Making disk: ${DISK_IMG} ${SIZE}K")
        set(TEMPLATE COMMAND "${DISKTOOL}" "${DISK_IMG}")
        set(CMDS "")
        set(DEPS "")
        foreach(n ${ARGN})
            set(CMDS ${CMDS} ${TEMPLATE} put "${n}")
            set(DEPS ${DEPS} "${n}")
        endforeach()
        add_custom_command(
            OUTPUT "${DISK_IMG}"
            ${TEMPLATE} create ${SIZE}
            ${TEMPLATE} boot "${BINARY_DIR}/boot.bin"
            ${TEMPLATE} put "${BINARY_DIR}/os.sys"
            ${TEMPLATE} put "${BINARY_DIR}/cmdp.com"
            ${TEMPLATE} put "${BINARY_DIR}/uvi.com"
            ${TEMPLATE} put "${BINARY_DIR}/debug.com"
            ${TEMPLATE} put "${BINARY_DIR}/sasm.com"
            ${TEMPLATE} put "${PROJECT_SOURCE_DIR}/LICENSE.md"
            ${CMDS}
            DEPENDS ${DEPS}
        )
        add_custom_target(${NAME} ALL DEPENDS "${DISK_IMG}")
    endmacro()
    set(NL "\n")
    if (NOT WIN32)
        set(NL "\r\n")
    endif()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/build.bat"
        "echo scc  scc${NL}scc scc.c${NL}"
        "echo scc  scc${NL}scc scc.c${NL}"
        "echo scc  scpp${NL}scc scpp.c${NL}"
        "echo scpp sim86${NL}scpp sim86.c${NL}"
        "echo scc  sim86${NL}scc sim86.i${NL}"
        "echo Run \"sim86 scc.com scc.c scc2.com\" to test${NL}"
        )
    set(SMALL_CONTENTS
        "${CMAKE_CURRENT_SOURCE_DIR}/scc.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/scpp.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sas.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sim86.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib.h"
        "${PROJECT_BINARY_DIR}/scc.com"
        "${PROJECT_BINARY_DIR}/scpp.com"
        "${PROJECT_BINARY_DIR}/sas.com"
        "${PROJECT_BINARY_DIR}/sim86.com"
        "${CMAKE_CURRENT_BINARY_DIR}/build.bat"
    )
    MAKEDISK(small 360 ${SMALL_CONTENTS})
    MAKEDISK(large 1440
        ${SMALL_CONTENTS}
        "${SOURCE_DIR}/strap.bat"
        "${SOURCE_DIR}/sasm.asm"
        "${SOURCE_DIR}/boot.asm"
        "${SOURCE_DIR}/os.asm"
        "${SOURCE_DIR}/cmdp.asm"
        "${SOURCE_DIR}/insboot.asm"
        "${SOURCE_DIR}/debug.asm"
        "${SOURCE_DIR}/uvi.asm"
        )

    find_program(QEMU qemu-system-i386)
    if (QEMU)
        message("Found qemu: ${QEMU}")
        set(QEMU_EXTRA_ARGS "" CACHE STRING "Extra arguments to give QEMU. E.g. \"-s -S\" or \"-curses\"")
        set(DISK_IMG "${CMAKE_CURRENT_BINARY_DIR}/large.img")
        add_custom_target(
            qemu_test
            COMMAND qemu-system-i386 -drive format=raw,file=\"${DISK_IMG}\",if=floppy ${QEMU_EXTRA_ARGS}
            DEPENDS ${DISK_IMG}
        )
    endif()
endif()
