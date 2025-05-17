add_library(wii_extensions wii.cpp)
target_include_directories(wii_extensions INTERFACE .)
target_link_libraries(wii_extensions PUBLIC
    peripherals
    midi
)