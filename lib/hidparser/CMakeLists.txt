add_library(hidparser INTERFACE)
target_include_directories(hidparser INTERFACE .)
target_sources(hidparser INTERFACE hidparser.c)