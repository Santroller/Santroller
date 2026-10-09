# Config storage and loading: the real src/config storage, flash cache and loader sources, compiled
# against the fakes in support/config (NOR flash in RAM, the managers ConfigLoader drives, and
# stand-ins for config.cpp's load_device / load_profile). Fixtures from the config tool's own
# encoder live in config/fixtures.
add_executable(config_tests
  ${SANTROLLER_ROOT}/src/config/config_storage.cpp
  ${SANTROLLER_ROOT}/src/config/config_loader.cpp
  ${SANTROLLER_ROOT}/src/config/FlashPROM.cpp
  ${SANTROLLER_ROOT}/src/devices/bt/bt_tlv_storage.cpp
  ${SANTROLLER_ROOT}/lib/CRC32/src/CRC32.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/support/config/fakes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/config_storage_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/flashprom_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/config_loader_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/aux_config_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/configurator_fixture_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/config/profile_opts_test.cpp
)
# The fakes come first so they shadow the firmware's own headers
target_include_directories(config_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support/config)
target_include_directories(config_tests PRIVATE
  ${SANTROLLER_ROOT}/lib/CRC32/src
)
target_compile_definitions(config_tests PRIVATE
  SANTROLLER_CONFIG_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config/fixtures"
)
target_compile_options(config_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(config_tests PRIVATE santroller_headers GTest::gtest_main)

gtest_discover_tests(config_tests DISCOVERY_MODE PRE_TEST)
