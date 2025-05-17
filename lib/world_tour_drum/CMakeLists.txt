add_library(world_tour_drum world_tour_drum.cpp)
target_include_directories(world_tour_drum INTERFACE .)
target_link_libraries(world_tour_drum PUBLIC
    peripherals
    midi
)