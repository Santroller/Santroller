add_library(gip_common INTERFACE)
target_sources(gip_common INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/gip_button_mapping.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gip_packet_handler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gip_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gip_report_queue.cpp
)
target_include_directories(gip_common INTERFACE ${CMAKE_CURRENT_LIST_DIR})
