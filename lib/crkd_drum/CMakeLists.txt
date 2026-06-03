add_library(crkd_drum INTERFACE)
target_include_directories(crkd_drum INTERFACE .)
target_link_libraries(crkd_drum INTERFACE
    peripherals
)
target_sources(crkd_drum INTERFACE crkd_drum_device.cpp)