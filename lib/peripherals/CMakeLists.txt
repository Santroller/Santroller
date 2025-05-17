add_library(peripherals i2c.cpp spi.cpp)
target_include_directories(peripherals INTERFACE .)
target_link_libraries(peripherals PUBLIC
    hardware_gpio
    hardware_i2c
    hardware_spi
)