add_library(psx_controller psx_controller.cpp)
target_include_directories(psx_controller INTERFACE .)
target_link_libraries(psx_controller PUBLIC
    peripherals
)