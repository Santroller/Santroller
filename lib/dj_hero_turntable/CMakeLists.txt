add_library(dj_hero_turntable dj_hero_turntable.cpp)
target_include_directories(dj_hero_turntable INTERFACE .)
target_link_libraries(dj_hero_turntable PUBLIC
    peripherals
)