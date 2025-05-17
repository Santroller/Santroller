add_library(guitar_hero_5_neck guitar_hero_5_neck.cpp)
target_include_directories(guitar_hero_5_neck INTERFACE .)
target_link_libraries(guitar_hero_5_neck PUBLIC
    peripherals
)