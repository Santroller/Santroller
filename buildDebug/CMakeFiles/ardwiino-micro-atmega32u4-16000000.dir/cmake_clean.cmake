file(REMOVE_RECURSE
  "../src/avr/micro/main/obj/ardwiino-micro-atmega32u4-16000000"
  "firmware/ardwiino-micro-atmega32u4-16000000.bin"
  "firmware/ardwiino-micro-atmega32u4-16000000.eep"
  "firmware/ardwiino-micro-atmega32u4-16000000.elf"
  "firmware/ardwiino-micro-atmega32u4-16000000.hex"
  "firmware/ardwiino-micro-atmega32u4-16000000.lss"
  "firmware/ardwiino-micro-atmega32u4-16000000.map"
  "firmware/ardwiino-micro-atmega32u4-16000000.sym"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/ardwiino-micro-atmega32u4-16000000.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
