if (MSVC)
    set(CAT type 2>nul:)
    set(RM del 2>nul:)
else()
    set(CAT cat)
    set(RM rm -f)
endif()

add_subdirectory(scc)

add_custom_target(test
    DEPENDS test_scc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Running tests
    )
