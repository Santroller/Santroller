add_library(uzlib INTERFACE)

target_sources(uzlib INTERFACE
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src/tinflate.c
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src/tinfgzip.c
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src/tinfzlib.c
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src/adler32.c
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src/crc32.c
)

target_include_directories(uzlib INTERFACE
    ${CMAKE_SOURCE_DIR}/lib/uzlib/src
)