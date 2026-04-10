add_library(ring_buffer INTERFACE)
target_include_directories(ring_buffer INTERFACE .)
target_link_libraries(ring_buffer INTERFACE
    pico_stdlib
)
target_sources(ring_buffer INTERFACE ring_buffer.c)