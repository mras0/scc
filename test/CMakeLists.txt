set(TESTS
    comma
    condop
    decl
    dowhile
    enum
    lgoto # goto would clash with DOS goto builtin
    sizeof
    struct
    switch
    union
    )

set(TEST_CMDS "echo off\nscc.com scc.c\nscc.com scc.c\n")
set(TEST_SOURCES "")
set(TEST_CLEAN "")

set(CAT cat)
set(RM rm -f)
if (MSVC)
    set(CAT type 2>nul:)
    set(RM del 2>nul:)
endif()

foreach(TEST ${TESTS})
    add_custom_target(${TEST}_c
        COMMAND ${CAT} testlib.c ${TEST}.c > "${CMAKE_CURRENT_BINARY_DIR}/${TEST}.c"
        DEPENDS testlib.c ${TEST}.c
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    set(TEST_CMDS "${TEST_CMDS}scc.com ${TEST}.c\n${TEST}.com\n")
    set(TEST_CLEAN ${TEST_CLEAN} ${TEST}.com)
    set(TEST_SOURCES ${TEST_SOURCES} ${TEST}_c)
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/runtests.bat" "${TEST_CMDS}")

add_custom_target(test
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/../scc.com" ./
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" ./
    COMMAND ${RM} ${TEST_CLEAN}
    COMMAND dosbox runtests.bat
    DEPENDS scc_com ${TEST_SOURCES} "${PROJECT_SOURCE_DIR}/scc.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Running tests
    )
