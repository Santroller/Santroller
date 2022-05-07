# Install script for directory: /home/sanjay/Code/Ardwiino

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
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/sanjay/Code/Ardwiino/pico_extras/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-adafruit_feather_rp2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-adafruit_itsybitsy_rp2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-adafruit_qtpy_rp2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-adafruit_trinkey_qt2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-arduino_nano_rp2040_connect/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-melopero_shake_rp2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pico/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_interstate75/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_keybow2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_pga2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_picolipo_16mb/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_picolipo_4mb/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_picosystem/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_plasma2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pimoroni_tiny2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-pybstick26_rp2040/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-sparkfun_micromod/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-sparkfun_promicro/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-sparkfun_thingplus/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-vgaboard/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-waveshare_rp2040_lcd_0/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-waveshare_rp2040_plus_16mb/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-waveshare_rp2040_plus_4mb/cmake_install.cmake")
  include("/home/sanjay/Code/Ardwiino/pico-sdk-waveshare_rp2040_zero/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/sanjay/Code/Ardwiino/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
