/*
 * ----------------------------------------------------------------------------
 * “THE COFFEEWARE LICENSE” (Revision 1):
 * <ihsan@kehribar.me> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a coffee in return.
 * -----------------------------------------------------------------------------
 * This library is based on this library:
 *   https://github.com/aaronds/arduino-nrf24l01
 * Which is based on this library:
 *   http://www.tinkerer.eu/AVRLib/nRF24L01
 * -----------------------------------------------------------------------------
 */
#ifndef NRF24
#define NRF24

#include <stdint.h>
#include <stdbool.h>

#define LOW 0
#define HIGH 1

#define nrf24_ADDR_LEN 4
#define nrf24_CONFIG ((1 << EN_CRC) | (0 << CRCO))

#define NRF24_TRANSMISSON_OK 0
#define NRF24_MESSAGE_LOST 1

/* adjustment functions */
void nrf24_init(void);
void nrf24_rx_address(uint8_t *adr);
void nrf24_tx_address(uint8_t *adr);
void nrf24_config(uint8_t channel, bool tx);

/* state check functions */
uint8_t nrf24_dataReady(void);
uint8_t nrf24_isSending(void);
uint8_t nrf24_getStatus(void);
uint8_t nrf24_rxFifoEmpty(void);
uint8_t nrf24_txFifoEmpty(void);
uint8_t nrf24_txFifoFull(void);

/* core TX / RX functions */
void nrf24_send(uint8_t *value, uint8_t payload_len);
int nrf24_getData(uint8_t *data, uint8_t payload_len);

/* use in dynamic length mode */
uint8_t nrf24_payloadLength(void);

/* post transmission analysis */
uint8_t nrf24_lastMessageStatus(void);
uint8_t nrf24_retransmissionCount(void);

/* power management */
void nrf24_powerUpRx(void);
void nrf24_powerUpTx(void);
void nrf24_powerDown(void);

void nrf24_flush_rx(void);
void nrf24_flush_tx(void);

void nrf24_send_init(void);
void nrf24_writeAckPayload(uint8_t* value, uint8_t size);

/* low level interface ... */
uint8_t spi_transfer(uint8_t tx);
void nrf24_transmitSync(uint8_t *dataout, uint8_t len);
void nrf24_transferSync(uint8_t *dataout, uint8_t *datain, uint8_t len);
void nrf24_configRegister(uint8_t reg, uint8_t value);
void nrf24_readRegister(uint8_t reg, uint8_t *value, uint8_t len);
uint8_t nrf24_readRegister1(uint8_t reg);
void nrf24_writeRegister(uint8_t reg, uint8_t *value, uint8_t len);

/* -------------------------------------------------------------------------- */
/* You should implement the platform spesific functions in your code */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* nrf24 CE pin control function
 *    - state:1 => Pin HIGH
 *    - state:0 => Pin LOW     */
/* -------------------------------------------------------------------------- */
extern void nrf24_ce_digitalWrite(uint8_t state);

/* -------------------------------------------------------------------------- */
/* nrf24 CE pin control function
 *    - state:1 => Pin HIGH
 *    - state:0 => Pin LOW     */
/* -------------------------------------------------------------------------- */
extern void nrf24_csn_digitalWrite(uint8_t state);

#endif
