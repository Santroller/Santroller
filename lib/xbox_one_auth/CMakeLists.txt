add_library(xbox_one_auth INTERFACE)
target_include_directories(xbox_one_auth INTERFACE .)
target_link_libraries(xbox_one_auth INTERFACE
    peripherals
)
target_sources(xbox_one_auth INTERFACE libxbox_one_auth.cpp)
