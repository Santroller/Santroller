add_library(display INTERFACE)
target_include_directories(display INTERFACE .)
target_link_libraries(display INTERFACE
    peripherals
)
target_sources(display INTERFACE libdisplay.cpp mono_framebuffer.cpp)
