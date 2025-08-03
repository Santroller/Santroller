add_library(xsm3 INTERFACE)
target_include_directories(xsm3 INTERFACE .)
target_link_libraries(xsm3 INTERFACE
    pico_stdlib
)
target_sources(xsm3 INTERFACE 
  excrypt_des.c
  excrypt_parve.c
  excrypt_sha.c
  usbdsec.c
  xsm3.c)