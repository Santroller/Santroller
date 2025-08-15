add_library(crkd_neck INTERFACE)
target_include_directories(crkd_neck INTERFACE .)
target_link_libraries(crkd_neck INTERFACE
    peripherals
)
target_sources(crkd_neck INTERFACE crkd_neck.cpp)