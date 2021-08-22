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
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "spi/spi.h"
#include "util/util.h"
// #include <avr/io.h>
// #include <util/delay.h>
#include "timer/timer.h"

bool is_tx;

/* init the hardware pins */
void nrf24_init(void) {
  spi_begin(F_CPU, false, false, false);
  nrf24_ce_digitalWrite(LOW);
  nrf24_csn_digitalWrite(HIGH);
  _delay_ms(5);
}
void nrf24_flush_tx(void) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(FLUSH_TX);
  nrf24_csn_digitalWrite(HIGH);
}
void nrf24_flush_rx(void) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(FLUSH_RX);
  nrf24_csn_digitalWrite(HIGH);
}
void nrf24_set_pa(pa_t pa) {
  uint8_t val;
  nrf24_readRegister(RF_SETUP, &val, 1);
  val &= ~(_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));
  if (pa == RF_PA_MAX) {
    val |= (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));
  } else if (pa == RF_PA_HIGH) {
    val |= _BV(RF_PWR_HIGH);
  } else if (pa == RF_PA_LOW) {
    val |= _BV(RF_PWR_LOW);
  }
  nrf24_configRegister(RF_SETUP, val);
}
bool p_type = false;
bool wide_band = false;
bool nrf24_setDataRate(dataRate_t rate) {
  uint8_t val;
  nrf24_readRegister(RF_SETUP, &val, 1);
  val &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));
  if (rate == RF_250KBPS) {
    val |= RF_DR_LOW;
    wide_band = false;
  } else if (rate == RF_2MBPS) {
    val |= RF_DR_HIGH;
    wide_band = true;
  } else {
    wide_band = false;
  }
  nrf24_configRegister(RF_SETUP, val);
  uint8_t val2;
  nrf24_readRegister(RF_SETUP, &val2, 1);
  if (val2 == val) { return true; }
  wide_band = false;
  return false;
}
void nrf24_setCRCLen(crclen_t length) {
  uint8_t config_reg;

  nrf24_readRegister(CONFIG, &config_reg, 1);
  config_reg &= ~(_BV(CRCO) | _BV(EN_CRC));

  if (length == RF_CRC_DISABLED) {
    // Do nothing, we turned it off above.
  } else if (length == RF_CRC_8) {
    config_reg |= _BV(EN_CRC);
  } else {
    config_reg |= _BV(EN_CRC);
    config_reg |= _BV(CRCO);
  }
  nrf24_configRegister(CONFIG, config_reg);
}
void nrf24_toggle_features(void) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(ACTIVATE);
  spi_transfer(0x73);
  nrf24_csn_digitalWrite(HIGH);
}

void nrf24_enable_ack_payload(void) {
  nrf24_configRegister(FEATURE, nrf24_readRegister1(FEATURE) | _BV(EN_DPL));
  nrf24_configRegister(DYNPD, nrf24_readRegister1(DYNPD) | _BV(DPL_P5) |
                                  _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) |
                                  _BV(DPL_P1) | _BV(DPL_P0));
  // Ack payloads and dynamic payload length (required for ack payloads)
  nrf24_configRegister(FEATURE, nrf24_readRegister1(FEATURE) | _BV(EN_ACK_PAY) |
                                    _BV(EN_DPL));
  nrf24_configRegister(DYNPD,
                       nrf24_readRegister1(DYNPD) | _BV(DPL_P0) | _BV(DPL_P1));
}
/* configure the module */
void nrf24_config(uint8_t channel, bool tx) {
  is_tx = tx;

  // Auto retransmit delay: 1000 us and no retransmits
  nrf24_configRegister(SETUP_RETR, (0x0F << ARD) | (0x01 << ARC));
  // Four id bytes (uint32_t)
  nrf24_configRegister(SETUP_AW, 0b10);
  nrf24_set_pa(RF_PA_MIN);

  // Only p_type modules support this
  uint8_t before_toggle = nrf24_readRegister1(FEATURE);
  nrf24_toggle_features();
  uint8_t after_toggle = nrf24_readRegister1(FEATURE);
  p_type = before_toggle == after_toggle;
  if (after_toggle) {
    if (p_type) {
      // module did not experience power-on-reset (#401)
      nrf24_toggle_features();
    }
    // allow use of multicast parameter and dynamic payloads by default
    nrf24_configRegister(FEATURE, 0);
  }

  nrf24_setDataRate(RF_2MBPS);

  // Initialize CRC and request 2-byte (16bit) CRC
  nrf24_setCRCLen(RF_CRC_16);
  // Set RF channel
  nrf24_configRegister(RF_CH, channel);
  nrf24_enable_ack_payload();

  // Auto Acknowledgment
  nrf24_configRegister(EN_AA, (1 << ENAA_P0) | (1 << ENAA_P1) | (1 << ENAA_P2) |
                                  (1 << ENAA_P3) | (1 << ENAA_P4) |
                                  (1 << ENAA_P5));
  // Reset current status
  nrf24_configRegister(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

  // Flush
  nrf24_flush_rx();
  nrf24_flush_tx();
  if (tx) {
    // // /* Set to transmitter mode , Power up if needed */
    nrf24_powerUpTx();
  } else {
    // Start listening
    nrf24_powerUpRx();
  }
}
/* Set the RX address */
void nrf24_rx_address(uint8_t *adr) {
  nrf24_ce_digitalWrite(LOW);
  nrf24_writeRegister(RX_ADDR_P1, adr, nrf24_ADDR_LEN);
  // Enable RX addresses
  nrf24_configRegister(EN_RXADDR, (1 << ERX_P0) | (1 << ERX_P1) |
                                      (0 << ERX_P2) | (0 << ERX_P3) |
                                      (0 << ERX_P4) | (0 << ERX_P5));
  nrf24_ce_digitalWrite(HIGH);
}

/* Set the TX address */
void nrf24_tx_address(uint8_t *adr) {
  /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
  nrf24_writeRegister(RX_ADDR_P0, adr, nrf24_ADDR_LEN);
  nrf24_writeRegister(TX_ADDR, adr, nrf24_ADDR_LEN);
}

/* Checks if data is available for reading */
/* Returns 1 if data is ready ... */
uint8_t nrf24_dataReady(void) {
  // See note in getData() function - just checking RX_DR isn't good enough
  uint8_t status = nrf24_getStatus();

  // We can short circuit on RX_DR, but if it's not set, we still need
  // to check the FIFO for any pending packets
  if (status & (1 << RX_DR)) { return 1; }

  return !nrf24_rxFifoEmpty();
  ;
}

/* Checks if receive FIFO is empty or not */
uint8_t nrf24_rxFifoEmpty(void) {
  uint8_t fifoStatus;

  nrf24_readRegister(FIFO_STATUS, &fifoStatus, 1);

  return (fifoStatus & (1 << RX_EMPTY));
}

/* Checks if receive FIFO is empty or not */
uint8_t nrf24_txFifoEmpty(void) {
  uint8_t fifoStatus;

  nrf24_readRegister(FIFO_STATUS, &fifoStatus, 1);

  return (fifoStatus & (1 << TX_EMPTY));
}

/* Checks if receive FIFO is empty or not */
uint8_t nrf24_txFifoFull(void) {;

  return (nrf24_getStatus() & (1 << TX_FULL));
}

/* Returns the length of data waiting in the RX fifo */
uint8_t nrf24_payloadLength(void) {
  uint8_t status;
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(R_RX_PL_WID);
  status = spi_transfer(0x00);
  nrf24_csn_digitalWrite(HIGH);
  return status;
}
/* Reads payload bytes into data array */
int nrf24_getData(uint8_t *data, uint8_t len) {
  /* Reset status register */
  nrf24_configRegister(STATUS, (1 << RX_DR));

  if (!len) {
    len = nrf24_payloadLength();
  }
  /* Pull down chip select */
  nrf24_csn_digitalWrite(LOW);
  /* Send cmd to read rx payload */
  spi_transfer(R_RX_PAYLOAD);

  /* Read payload */
  nrf24_transferSync(data, data, len);

  /* Pull up chip select */
  nrf24_csn_digitalWrite(HIGH);
  return len;
}

/* Returns the number of retransmissions occured for the last message */
uint8_t nrf24_retransmissionCount(void) {
  uint8_t rv;
  nrf24_readRegister(OBSERVE_TX, &rv, 1);
  rv = rv & 0x0F;
  return rv;
}

// Sends a data package to the default address. Be sure to send the correct
// amount of bytes as configured as payload on the receiver.
void nrf24_send(uint8_t *value, uint8_t payload_len) {

  // // /* Go to Standby-I first */
  // nrf24_ce_digitalWrite(LOW);

  nrf24_configRegister(STATUS, _BV(TX_DS) | _BV(MAX_RT));
  // // Max retries exceeded
  // if (nrf24_getStatus() & _BV(MAX_RT)) {
  //   nrf24_flush_tx(); // Only going to be 1 packet int the FIFO at a time using
  //                     // this method, so just flush
  // }

  /* Pull down chip select */
  nrf24_csn_digitalWrite(LOW);

  /* Write cmd to write payload */
  spi_transfer(W_TX_PAYLOAD);

  /* Write payload */
  nrf24_transmitSync(value, payload_len);

  /* Pull up chip select */
  nrf24_csn_digitalWrite(HIGH);

  // /* Start the transmission */
  // nrf24_ce_digitalWrite(HIGH);
}

uint8_t nrf24_isSending(void) {
  uint8_t status;

  /* read the current status */
  status = nrf24_getStatus();

  /* if sending successful (TX_DS) or max retries exceded (MAX_RT). */
  if ((status & ((1 << TX_DS) | (1 << MAX_RT)))) { return 0; /* false */ }

  return 1; /* true */
}

uint8_t nrf24_getStatus(void) {
  uint8_t rv;
  nrf24_csn_digitalWrite(LOW);
  rv = spi_transfer(NOP);
  nrf24_csn_digitalWrite(HIGH);
  return rv;
}

uint8_t nrf24_lastMessageStatus(void) {
  uint8_t rv;

  rv = nrf24_getStatus();

  /* Transmission went OK */
  if ((rv & ((1 << TX_DS)))) {
    return NRF24_TRANSMISSON_OK;
  }
  /* Maximum retransmission count is reached */
  /* Last message probably went missing ... */
  else if ((rv & ((1 << MAX_RT)))) {
    return NRF24_MESSAGE_LOST;
  }
  /* Probably still sending ... */
  else {
    return 0xFF;
  }
}

void nrf24_powerUpRx(void) {
  nrf24_configRegister(STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));
  uint8_t config_reg = 0;
  nrf24_readRegister(CONFIG, &config_reg, 1);
  config_reg &= ~(_BV(PRIM_RX) | _BV(PWR_UP) | _BV(MASK_RX_DR) |
                  _BV(MASK_TX_DS) | _BV(MASK_MAX_RT));
  nrf24_ce_digitalWrite(LOW);
  nrf24_configRegister(CONFIG, config_reg | ((1 << PWR_UP) | (1 << PRIM_RX)));
  nrf24_ce_digitalWrite(HIGH);
}

void nrf24_powerUpTx(void) {
  uint8_t config_reg = 0;
  nrf24_readRegister(CONFIG, &config_reg, 1);
  config_reg &= ~(_BV(PRIM_RX) | _BV(PWR_UP) | _BV(MASK_RX_DR) |
                  _BV(MASK_TX_DS) | _BV(MASK_MAX_RT));
  nrf24_ce_digitalWrite(LOW);
  nrf24_configRegister(CONFIG, config_reg | ((1 << PWR_UP) | (0 << PRIM_RX)));
  nrf24_ce_digitalWrite(HIGH);
}

void nrf24_powerDown(void) {
  nrf24_ce_digitalWrite(LOW);
  uint8_t config_reg = 0;
  nrf24_readRegister(CONFIG, &config_reg, 1);
  bit_clear(config_reg, PWR_UP);
  nrf24_configRegister(CONFIG, config_reg);
}

/* send and receive multiple bytes over SPI */
void nrf24_transferSync(uint8_t *dataout, uint8_t *datain, uint8_t len) {
  uint8_t i;

  for (i = 0; i < len; i++) { datain[i] = spi_transfer(dataout[i]); }
}

/* send multiple bytes over SPI */
void nrf24_transmitSync(uint8_t *dataout, uint8_t len) {
  uint8_t i;

  for (i = 0; i < len; i++) { spi_transfer(dataout[i]); }
}

/* Clocks only one byte into the given nrf24 register */
void nrf24_configRegister(uint8_t reg, uint8_t value) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
  spi_transfer(value);
  nrf24_csn_digitalWrite(HIGH);
}

/* Read single register from nrf24 */
void nrf24_readRegister(uint8_t reg, uint8_t *value, uint8_t len) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
  nrf24_transferSync(value, value, len);
  nrf24_csn_digitalWrite(HIGH);
}

uint8_t nrf24_readRegister1(uint8_t reg) {
  uint8_t value;
  nrf24_readRegister(reg, &value, 1);
  return value;
}

/* Write to a single register of nrf24 */
void nrf24_writeRegister(uint8_t reg, uint8_t *value, uint8_t len) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
  nrf24_transmitSync(value, len);
  nrf24_csn_digitalWrite(HIGH);
}

void nrf24_writeAckPayload(uint8_t *buf, uint8_t size) {
  nrf24_csn_digitalWrite(LOW);
  spi_transfer(W_ACK_PAYLOAD | 1);
  /* Write payload */
  nrf24_transmitSync(buf, size);
  nrf24_csn_digitalWrite(HIGH);
}