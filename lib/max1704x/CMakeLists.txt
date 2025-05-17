add_library(max1704x max1704x.cpp)
target_include_directories(max1704x INTERFACE .)
target_link_libraries(max1704x PUBLIC
    peripherals
)