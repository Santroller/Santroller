file(REMOVE_RECURSE
  "../src/avr/micro/main/obj/ardwiino-micro-atmega32u4-8000000-multi"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.bin"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.eep"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.elf"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.hex"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.lss"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.map"
  "firmware/ardwiino-micro-atmega32u4-8000000-multi.sym"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/ardwiino-micro-atmega32u4-8000000-multi.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
