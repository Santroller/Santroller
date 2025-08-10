add_library(mpr121 INTERFACE)
target_include_directories(mpr121 INTERFACE .)
target_link_libraries(mpr121 INTERFACE
    peripherals
)
target_sources(mpr121 INTERFACE libmpr121.cpp)