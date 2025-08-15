add_library(peripherals INTERFACE)
target_include_directories(peripherals INTERFACE .)
target_link_libraries(peripherals INTERFACE
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_uart
)
target_sources(peripherals INTERFACE i2c.cpp spi.cpp uart.cpp)