cmake_minimum_required(VERSION 3.13)
project(ardwiino C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_PROCESSOR arm)
find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")
endif(GIT_FOUND)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmware)
# if BOARD is set, then we are building a specific pico board and can skip other boards
if (NOT BOARD)
  include(avr.cmake)
  return()
endif()
include(pico.cmake)
message("Building board=${BOARD}")
add_custom_target(pico)
add_dependencies(pico ardwiino-${BOARD}-rp2040)
add_custom_command(
  TARGET pico
  POST_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/bootloader.py)
add_custom_command(
    TARGET pico
    POST_BUILD
    COMMAND sleep 10)
add_custom_command(
  TARGET pico
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-pico-rp2040.uf2
    /run/media/$ENV{USERNAME}/RPI-RP2/)