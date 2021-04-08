SRC += ${PROJECT_ROOT}/src/avr/lib/timer/timer.c ${PROJECT_ROOT}/src/shared/output/serial_handler.c
SRC += ${PROJECT_ROOT}/src/shared/output/reports.c 
SRC += ${PROJECT_ROOT}/lib/mpu6050/inv_mpu_dmp_motion_driver.c ${PROJECT_ROOT}/lib/mpu6050/inv_mpu.c ${PROJECT_ROOT}/lib/mpu6050/mpu_math.c
SRC += ${PROJECT_ROOT}/src/avr/lib/spi/spi.c ${PROJECT_ROOT}/src/avr/lib/i2c/i2c.c ${PROJECT_ROOT}/src/avr/lib/pins/pins.c ${PROJECT_ROOT}/src/shared/leds/leds.c
SRC += ${PROJECT_ROOT}/src/shared/rf/rf.c ${PROJECT_ROOT}/src/shared/input/input_handler.c ${PROJECT_ROOT}/src/avr/lib/eeprom/eeprom.c
SRC += ${PROJECT_ROOT}/lib/avr-nrf24l01/src/nrf24l01.c ${PROJECT_ROOT}/src/shared/controller/guitar_includes.c ${PROJECT_ROOT}/src/shared/lib/i2c/i2c_shared.c
SRC += ${PROJECT_ROOT}/lib/fxpt_math/fxpt_math.c