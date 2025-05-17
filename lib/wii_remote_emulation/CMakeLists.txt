add_library(wii_remote_emulation INTERFACE)
target_include_directories(wii_remote_emulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(wii_remote_emulation INTERFACE pico_stdlib pico_btstack_classic pico_btstack_cyw43
  	pico_cyw43_arch_none)
target_sources(wii_remote_emulation INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/wiimote_btstack.c
    ${CMAKE_CURRENT_LIST_DIR}/wiimote.c
    ${CMAKE_CURRENT_LIST_DIR}/wm_reports.c
    ${CMAKE_CURRENT_LIST_DIR}/motion.c
    ${CMAKE_CURRENT_LIST_DIR}/wm_crypto.c
    ${CMAKE_CURRENT_LIST_DIR}/inet.c)
