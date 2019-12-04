if (MSVC)
    set(CAT type 2>nul:)
    set(RM del 2>nul:)
    set(DIFF comp /m)
else()
    set(CAT cat)
    set(RM rm -f)
    set(DIFF diff)
endif()

add_subdirectory(scc)
add_subdirectory(scpp)
add_subdirectory(sas)

add_custom_target(test
    DEPENDS test_scc test_scpp test_sas
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Running tests
    )
