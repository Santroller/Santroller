add_library(nanopb INTERFACE)

target_include_directories(nanopb INTERFACE .)
target_sources(nanopb INTERFACE
    pb_common.c
    pb_decode.c
    pb_encode.c
)