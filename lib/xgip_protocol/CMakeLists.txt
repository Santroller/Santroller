add_library(xgip_protocol INTERFACE)
target_include_directories(xgip_protocol INTERFACE .)
target_link_libraries(xgip_protocol INTERFACE
    pico_stdlib
)
target_sources(SNESpad INTERFACE xgip_protocol.cpp)