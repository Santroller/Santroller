add_library(max1704x INTERFACE)
target_include_directories(max1704x INTERFACE .)
target_link_libraries(max1704x INTERFACE
    peripherals
)
target_sources(max1704x INTERFACE libmax1704x.cpp)