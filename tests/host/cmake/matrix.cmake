add_executable(matrix_tests
  ${SANTROLLER_ROOT}/lib/matrix/libmatrix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/matrix/matrix_test.cpp
)
target_include_directories(matrix_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support/matrix
  ${SANTROLLER_ROOT}/lib/matrix
)
target_compile_options(matrix_tests PRIVATE -Wall -Wextra)
target_link_libraries(matrix_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(matrix_tests DISCOVERY_MODE PRE_TEST)
