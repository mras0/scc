add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test.com"
        COMMAND $<TARGET_FILE:sas> "${CMAKE_CURRENT_SOURCE_DIR}/test.asm" "${CMAKE_CURRENT_BINARY_DIR}/test.com"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test.asm"
    )

add_custom_target(test_sim86
    COMMAND $<TARGET_FILE:sim86> test.com
    # Check simulated self-compilation
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/scc.c" .
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/scc.com" .
    COMMAND $<TARGET_FILE:sim86> scc.com scc.c sc1.com
    COMMAND ${DIFF} sc1.com scc.com
    COMMAND $<TARGET_FILE:sim86> sc1.com scc.c sc2.com
    COMMAND ${DIFF} sc2.com scc.com
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/test.com" scc_com
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Testing sim86
    )


