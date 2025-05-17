add_library(psx_controller INTERFACE)
target_include_directories(psx_controller INTERFACE .)
target_link_libraries(psx_controller INTERFACE
    peripherals
)
target_sources(psx_controller INTERFACE psx_controller.cpp)