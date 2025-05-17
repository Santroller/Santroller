add_library(guitar_hero_5_neck INTERFACE)
target_include_directories(guitar_hero_5_neck INTERFACE .)
target_link_libraries(guitar_hero_5_neck INTERFACE
    peripherals
)
target_sources(guitar_hero_5_neck INTERFACE guitar_hero_5_neck.cpp)