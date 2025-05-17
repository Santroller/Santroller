add_library(band_hero_drum INTERFACE)
target_include_directories(band_hero_drum INTERFACE .)
target_link_libraries(band_hero_drum INTERFACE
    peripherals
    midi
)
target_sources(band_hero_drum INTERFACE band_hero_drum.cpp)