add_library(world_tour_drum INTERFACE)
target_include_directories(world_tour_drum INTERFACE .)
target_link_libraries(world_tour_drum INTERFACE
    peripherals
    midi
)
target_sources(world_tour_drum INTERFACE world_tour_drum.cpp)