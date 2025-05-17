add_library(dj_hero_turntable INTERFACE)
target_include_directories(dj_hero_turntable INTERFACE .)
target_link_libraries(dj_hero_turntable INTERFACE
    peripherals
)
target_sources(dj_hero_turntable INTERFACE dj_hero_turntable.cpp)