if (MSVC)
    set(CAT type 2>nul:)
    set(RM del 2>nul:)
    set(DIFF comp /m)
else()
    set(CAT cat)
    set(RM rm -f)
    set(DIFF diff)
endif()

add_executable(dossim dossim.c)

find_program(DOSBOX dosbox)
if (NOT DOSBOX)
    message(WARNING "DOSBox not found - Using simulator.")
    set(DOSBOX $<TARGET_FILE:dossim> "$<TARGET_FILE:sim86>")
endif()

add_subdirectory(scc)
add_subdirectory(scpp)
add_subdirectory(sas)
add_subdirectory(sim86)

add_custom_target(test
    DEPENDS test_scc test_scpp test_sas test_sim86
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT Running tests
    )
