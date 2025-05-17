add_library(mpr121 mpr121.cpp)
target_include_directories(mpr121 INTERFACE .)
target_link_libraries(mpr121 PUBLIC
    peripherals
)