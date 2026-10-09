# PS2 controller emulation, replayed against bus captures from real controllers
add_executable(psx_tests ${CMAKE_CURRENT_SOURCE_DIR}/psx/psx_replay_test.cpp)
target_include_directories(psx_tests PRIVATE ${SANTROLLER_ROOT}/lib/psx_emulation)
target_compile_definitions(psx_tests PRIVATE PSX_CAPTURE_CSV="${CMAKE_CURRENT_SOURCE_DIR}/psx/data/ps2_captures.csv")
target_compile_options(psx_tests PRIVATE -Wall -Wextra)
target_link_libraries(psx_tests PRIVATE santroller_headers GTest::gtest_main)
gtest_discover_tests(psx_tests DISCOVERY_MODE PRE_TEST)
