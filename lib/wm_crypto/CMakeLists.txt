add_library(wm_crypto INTERFACE)
target_include_directories(wm_crypto INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(wm_crypto INTERFACE pico_stdlib)
target_sources(wm_crypto INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/wm_crypto.c)
