add_library(SNESpad INTERFACE)
target_link_libraries(SNESpad INTERFACE pico_stdlib)
target_include_directories(SNESpad INTERFACE .)
target_sources(SNESpad INTERFACE SNESpad.cpp)