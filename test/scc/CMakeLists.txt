set(TESTS
    array
    comma
    condop
    deadcode
    decl
    dowhile
    enum
    lgoto # goto would clash with DOS goto builtin
    sizeof
    struct
    switch
    union
    bug
    strlit
    unsigned
    typedef
    )

set(TEST_CMDS "echo off\nscc.com scc.c\nscc.com scc.c\n")
set(TEST_SOURCES "")
set(TEST_CLEAN "")
set(SIM_CMDS "")

foreach(TEST ${TESTS})
    add_custom_target(${TEST}_c
        COMMAND ${CAT} testlib.c ${TEST}.c > "${CMAKE_CURRENT_BINARY_DIR}/${TEST}.c"
        DEPENDS testlib.c ${TEST}.c
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    set(TEST_CMDS "${TEST_CMDS}scc.com ${TEST}.c\n${TEST}.com\n")
    set(TEST_CLEAN ${TEST_CLEAN} ${TEST}.com)
    set(TEST_SOURCES ${TEST_SOURCES} ${TEST}_c)
    set(SIM_CMDS ${SIM_CMDS} COMMAND $<TARGET_FILE:sim86> ${TEST}.com)
endforeach()

# Check that v1.0 can still be compiled and self-compile
set(TEST_CMDS "${TEST_CMDS}scc.com scc10.c scc10.com\nscc10.com scc10.c scc10b.com\n")
set(TEST_CLEAN ${TEST_CLEAN} scc10.com scc10b.com)
set(SIM_CMDS ${SIM_CMDS} COMMAND ${DIFF} \"${CMAKE_CURRENT_BINARY_DIR}/scc10b.com\" \"${CMAKE_CURRENT_SOURCE_DIR}/scc10.ref\")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/runtests.bat" "${TEST_CMDS}")

add_custom_target(test_scc
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/scc.com" ./
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" ./
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/scc10.c" ./
    COMMAND ${RM} ${TEST_CLEAN}
    COMMAND ${DOSBOX} runtests.bat
    ${SIM_CMDS}
    DEPENDS scc_com ${TEST_SOURCES} "${PROJECT_SOURCE_DIR}/scc.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Testing scc
    )
