add_library(protar_neck INTERFACE)
target_include_directories(protar_neck INTERFACE .)
target_link_libraries(protar_neck INTERFACE
    peripherals
)
target_sources(protar_neck INTERFACE protar_neck_device.cpp)