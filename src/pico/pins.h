#pragma once
#define PIN_WIRE_SDA 18
#define PIN_WIRE_SCL 19
#define PIN_SPI_MOSI 3
#define PIN_SPI_MISO 4
#define PIN_SPI_SCK 6
#define PIN_PS2_ACK 7
#define PIN_PS2_ATT 10
#define PIN_RF_IRQ 7
#define PIN_WAKEUP 8
#define PIN_USB_DP_PIN 20
// RF pins
#define PIN_SPI_SS 5
#define CE 8
#define CSN PIN_SPI_SS

void initPins(void);
void initDetectionDigital(void);
void initDetectionAnalog(void);
int detectAnalog();
void detectDigital(uint8_t* mask, uint8_t* pin);
void tickPins(void);
void stopReading(void);
int16_t adc(uint8_t analogIndex, int16_t offset, int16_t multiplier, int16_t deadzone);
uint16_t adc_trigger(uint8_t analogIndex, int16_t offset, int16_t multiplier, int16_t deadzone);