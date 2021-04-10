# Install script for directory: /home/sanjay/Code/Ardwiino/submodules/pico-extras/src/rp2_common

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/sbin/arm-none-eabi-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/hardware_rosc/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/lwip/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_sleep/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_audio_i2s/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_audio_pwm/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_audio_spdif/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_sd_card/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/pico_scanvideo_dpi/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/usb_common/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/usb_device/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/buildDebug/pico_extras/src/rp2_common/usb_device_msc/cmake_install.cmake")

endif()

