add_library(secondary_pico secondary_pico.cpp)
target_include_directories(secondary_pico INTERFACE .)
target_link_libraries(secondary_pico PUBLIC
    peripherals
)