// ===================================================================================
// nRF24L01+ Functions                                                        * v1.0 *
// ===================================================================================
//
// 2023 by Stefan Wagner:   https://github.com/wagiminator

#include "nrf24l01.h"
#include "spi.h"
#include "delay.h"

// ===================================================================================
// nRF24L01+ Implementation - Definitions and Variables
// ===================================================================================

// NRF registers
#define NRF_REG_CONFIG        0x00              // configuration register
#define NRF_REG_EN_AA         0x01              // Auto Ack enable
#define NRF_REG_SETUP_AW      0x03              // Address width register
#define NRF_REG_SETUP_RETR    0x04              // Transmit control
#define NRF_REG_RF_CH         0x05              // RF frequency channel
#define NRF_REG_RF_SETUP      0x06              // RF setup register
#define NRF_REG_STATUS        0x07              // status register
#define NRF_REG_RX_ADDR_P0    0x0A              // RX address pipe 0
#define NRF_REG_RX_ADDR_P1    0x0B              // RX address pipe 1
#define NRF_REG_TX_ADDR       0x10              // TX address
#define NRF_REG_FIFO_STATUS   0x17              // FIFO status register
#define NRF_REG_DYNPD         0x1C              // enable dynamic payload length
#define NRF_REG_FEATURE       0x1D              // feature

// NRF commands
#define NRF_CMD_R_RX_PL_WID   0x60              // read RX payload length
#define NRF_CMD_R_RX_PAYLOAD  0x61              // read RX payload
#define NRF_CMD_W_TX_PAYLOAD  0xA0              // write TX payload
#define NRF_CMD_FLUSH_TX      0xE1              // flush TX FIFO
#define NRF_CMD_FLUSH_RX      0xE2              // flush RX FIFO

// NRF global variables
__xdata uint8_t NRF_channel   = 0x02;           // channel (0x00 - 0x7F)
__xdata uint8_t NRF_speed     = 0;              // 0:250kbps, 1:1Mbps, 2:2Mbps
__xdata uint8_t NRF_tx_addr[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
__xdata uint8_t NRF_rx_addr[] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
__code uint8_t  NRF_SETUP[]   = {0x26, 0x06, 0x0E};
__code uint8_t* NRF_STR[]     = {"250k", "1M", "2M"};
__xdata options_t options = 0;

// ===================================================================================
// nRF24L01+ Implementation - SPI Communication Functions
// ===================================================================================

// NRF setup
void NRF_init(void) {
  SPI_init();
  NRF_configure();

  #ifdef USE_NRF_INT
  IE_GPIO = 1;
  GPIO_IE = bIE_IO_EDGE | bIE_P3_1_LO;
  #endif
}

// NRF send a command
void NRF_writeCommand(uint8_t cmd) {
  PIN_low(PIN_CSN);
  SPI_transfer(cmd);
  PIN_high(PIN_CSN);
}

// NRF write one byte into the specified register
void NRF_writeRegister(uint8_t reg, uint8_t value) {
  PIN_low(PIN_CSN);
  SPI_transfer(reg + 0x20);
  SPI_transfer(value);
  PIN_high(PIN_CSN);
}

// NRF read one byte from the specified register
uint8_t NRF_readRegister(uint8_t reg) {
  uint8_t value;
  PIN_low(PIN_CSN);
  SPI_transfer(reg);
  value = SPI_transfer(0);
  PIN_high(PIN_CSN);
  return value;
}

// NRF write an array of bytes into the specified registers
void NRF_writeBuffer(uint8_t reg, __xdata uint8_t *buf, uint8_t len) {
  if(reg < 0x20) reg += 0x20;
  PIN_low(PIN_CSN);
  SPI_transfer(reg);
  while(len--) SPI_transfer(*buf++);
  PIN_high(PIN_CSN);
}

// NRF read an array of bytes from the specified registers
void NRF_readBuffer(uint8_t reg, __xdata uint8_t *buf, uint8_t len) {
  PIN_low(PIN_CSN);
  SPI_transfer(reg);
  while(len--) *buf++ = SPI_transfer(0);
  PIN_high(PIN_CSN);
}

// ===================================================================================
// nRF24L01+ Implementation - Transceiver Functions
// ===================================================================================

// NRF switch to Power Down
void NRF_powerDown(void) {
  PIN_low(PIN_CE);                                      // return to Standby-I
  NRF_writeRegister(NRF_REG_CONFIG, NRF_CONFIG | 0x00); // !PWR_UP
}

// NRF switch to RX mode
void NRF_powerRX(void) {
  PIN_low(PIN_CE);                                      // return to Standby-I
  //DLY_us(100);
  NRF_writeRegister(NRF_REG_CONFIG, NRF_CONFIG | 0x03); // PWR_UP + PRIM_RX
  PIN_high(PIN_CE);                                     // switch to RX Mode
  DLY_us(200);
}

// NRF switch to TX mode
void NRF_powerTX(void) {
  PIN_low(PIN_CE);                                      // return to Standby-I
  NRF_writeRegister(NRF_REG_CONFIG, NRF_CONFIG | 0x02); // PWR_UP + !PRIM_RX
  PIN_high(PIN_CE);                                     // switch to TX Mode
  DLY_us(200);
  //PIN_low(PIN_CE);
}

// NRF configure
void NRF_configure(void) {
  PIN_low(PIN_CE);                                      // leave active mode
  NRF_writeBuffer(NRF_REG_RX_ADDR_P1, NRF_rx_addr, 5);  // set RX address
  NRF_writeBuffer(NRF_REG_TX_ADDR,    NRF_tx_addr, 5);  // set TX address
  NRF_writeBuffer(NRF_REG_RX_ADDR_P0, NRF_tx_addr, 5);  // set TX address for auto-ACK
  NRF_writeRegister(NRF_REG_RF_CH,    NRF_channel);        // set channel
  NRF_writeRegister(NRF_REG_RF_SETUP, NRF_SETUP[NRF_speed]); // set speed and power
  NRF_writeRegister(NRF_REG_FEATURE,  0x04);            // enable dynamic payload length
  NRF_writeRegister(NRF_REG_DYNPD,    (options & DYNAMIC_PAYLOAD) ? 0x3F : 00);            // enable dynamic payload length
  NRF_writeRegister(NRF_REG_SETUP_AW, 0x03);            // Address width of 5
  NRF_writeCommand(NRF_CMD_FLUSH_RX);                   // flush RX FIFO
  NRF_writeRegister(NRF_REG_EN_AA, (options & AUTO_ACK) ? 0x3F : 0x00);   // auto-ack all pipes
  NRF_writeRegister(NRF_REG_SETUP_RETR, 0x4F);
  NRF_powerRX();                                        // switch to RX Mode
}

//NOTE: Confirm configuration register (for testing)
uint8_t NRF_readconfig(void) {
  return(NRF_readRegister(NRF_REG_CONFIG));
}

//NOTE: Confirm status register (for testing)
uint8_t NRF_readstatus(void) {
  return(NRF_readRegister(NRF_REG_STATUS));
}

//NOTE: Confirm FIFO status register (for testing)
uint8_t NRF_readfifostatus(void) {
  return(NRF_readRegister(NRF_REG_FIFO_STATUS));
}

// Check if data is available for reading
uint8_t NRF_available(void) {
  if(NRF_readRegister(NRF_REG_STATUS) & 0x40) return 1;
  return(!(NRF_readRegister(NRF_REG_FIFO_STATUS) & 0x01));
}

// Read payload bytes into buffer, return payload length
uint8_t NRF_readPayload(__xdata uint8_t *buf) {
  uint8_t len = NRF_readRegister(NRF_CMD_R_RX_PL_WID);  // read payload length
  NRF_readBuffer(NRF_CMD_R_RX_PAYLOAD, buf, len);       // read payload
  NRF_writeRegister(NRF_REG_STATUS, 0x40);              // reset status register
  return len;                                           // return payload length
}

// Send a data package (max length 32)
void NRF_writePayload(__xdata uint8_t *buf, uint8_t len) {
  NRF_writeRegister(NRF_REG_STATUS, 0x30);              // clear status flags
  NRF_writeCommand(NRF_CMD_FLUSH_TX);                   // flush TX FIFO
  NRF_powerTX();                                        // switch to TX Mode
  NRF_writeBuffer(NRF_CMD_W_TX_PAYLOAD, buf, len);      // write payload and transmit
  while(!(NRF_readRegister(NRF_REG_STATUS) & 0x30));    // wait until finished
  NRF_powerRX();                                        // return to listening
}
