add_library(band_hero_drum band_hero_drum.cpp)
target_include_directories(band_hero_drum INTERFACE .)
target_link_libraries(band_hero_drum PUBLIC
    peripherals
    midi
)