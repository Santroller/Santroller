add_library(ads1115 INTERFACE)
target_include_directories(ads1115 INTERFACE .)
target_link_libraries(ads1115 INTERFACE
    peripherals
)
target_sources(ads1115 INTERFACE libads1115.cpp)