add_library(crazy_guitar_neck crazy_guitar_neck.cpp)
target_include_directories(crazy_guitar_neck INTERFACE .)
target_link_libraries(crazy_guitar_neck PUBLIC
    peripherals
)