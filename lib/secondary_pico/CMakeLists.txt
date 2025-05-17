add_library(secondary_pico INTERFACE)
target_include_directories(secondary_pico INTERFACE .)
target_link_libraries(secondary_pico INTERFACE
    peripherals
)
target_sources(secondary_pico INTERFACE secondary_pico.cpp)