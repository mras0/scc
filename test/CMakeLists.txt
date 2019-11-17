set(TESTS
    comma
    dowhile
    enum
    lgoto # goto would clash with DOS goto builtin
    struct
    switch
    )

set(TESTCMDS "scc.com scc.c\nscc.com scc.c\n")
set(TEST_SOURCES "")

set(CAT cat)
if (MSVC)
    set(CAT type 2>nul:)
endif()

foreach(TEST ${TESTS})
    add_custom_target(${TEST}_c
        COMMAND ${CAT} testlib.c ${TEST}.c > "${CMAKE_CURRENT_BINARY_DIR}/${TEST}.c"
        DEPENDS testlib.c ${TEST}.c
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    set(TESTCMDS "${TESTCMDS}scc.com ${TEST}.c\n${TEST}.com\n")
    set(TEST_SOURCES ${TEST_SOURCES} ${TEST}_c)
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/runtests.bat" "${TESTCMDS}")

add_custom_target(test
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/../scc.com" ./
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" ./
    COMMAND dosbox runtests.bat
    DEPENDS scc_com ${TEST_SOURCES} "${PROJECT_SOURCE_DIR}/scc.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Running tests
    )
