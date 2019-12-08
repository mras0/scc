set(TESTS
    sasm
    scc-self
    )

set(TEST_CMDS "echo off\nscc.com scc.c\nscc.com sas.c\n")
set(TEST_SOURCES "")
set(TEST_CLEAN "")
set(TEST_COMPARE "")

foreach(TEST ${TESTS})
    set(TEST_CMDS "${TEST_CMDS}sas.com ${TEST}.asm\n")
    set(TEST_CLEAN ${TEST_CLEAN} ${TEST}.com)
    set(TEST_SOURCES ${TEST_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/${TEST}.asm")
    set(TEST_COMPARE ${TEST_COMPARE} COMMAND ${DIFF} ${TEST}.com \"${CMAKE_CURRENT_SOURCE_DIR}/${TEST}.ref\")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/runtests.bat" "${TEST_CMDS}")

add_custom_target(test_sas
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_SOURCES} .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/scc.com" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/sas.c" .
    COMMAND ${RM} ${TEST_CLEAN}
    COMMAND ${DOSBOX} runtests.bat
    ${TEST_COMPARE}
    DEPENDS scc_com ${TEST_SOURCES} "${PROJECT_SOURCE_DIR}/sas.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Testing sas
    )

