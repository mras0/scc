
set(TEST_CMDS "echo off\nscc.com scc.c\nscc.com scpp.c\n")
set(TEST_SOURCES "")
set(TEST_CLEAN "")
set(TEST_COMPARE "")

file(GLOB TESTS "${CMAKE_CURRENT_SOURCE_DIR}/*.in")
foreach(SRC ${TESTS})
    get_filename_component(TEST ${SRC} NAME_WE)
    set(TEST_CMDS "${TEST_CMDS}scpp.com ${TEST}.in\n")
    set(TEST_CLEAN ${TEST_CLEAN} ${TEST}.i)
    set(TEST_SOURCES ${TEST_SOURCES} ${SRC})
    set(TEST_COMPARE ${TEST_COMPARE} COMMAND ${DIFF} ${TEST}.i \"${CMAKE_CURRENT_SOURCE_DIR}/${TEST}.ref\")
endforeach()

# For now just check that the files can be processed w/o errors
set(TEST_CMDS "${TEST_CMDS}scpp.com scc.c\n")
set(TEST_CMDS "${TEST_CMDS}scpp.com scpp.c\n")
set(TEST_CMDS "${TEST_CMDS}scpp.com sas.c\n")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/runtests.bat" "${TEST_CMDS}")

set(TEST_SOURCES ${TEST_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/inc1.h)

add_custom_target(test_scpp
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_SOURCES} .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/scc.com" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scpp.c" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/sas.c" .
    COMMAND ${RM} ${TEST_CLEAN}
    COMMAND dosbox runtests.bat
    ${TEST_COMPARE}
    DEPENDS scc_com ${TEST_SOURCES} "${PROJECT_SOURCE_DIR}/scpp.c" "${PROJECT_SOURCE_DIR}/sas.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Testing scpp
    )


