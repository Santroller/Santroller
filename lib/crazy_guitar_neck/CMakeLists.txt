add_library(crazy_guitar_neck INTERFACE)
target_include_directories(crazy_guitar_neck INTERFACE .)
target_link_libraries(crazy_guitar_neck INTERFACE
    peripherals
)
target_sources(crazy_guitar_neck INTERFACE crazy_guitar_neck.cpp)