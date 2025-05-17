add_library(wii_extensions INTERFACE)
target_include_directories(wii_extensions INTERFACE .)
target_link_libraries(wii_extensions INTERFACE
    peripherals
    midi
)
target_sources(wii_extensions INTERFACE wii.cpp)