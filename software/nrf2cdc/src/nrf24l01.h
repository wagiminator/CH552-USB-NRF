// ===================================================================================
// nRF24L01+ Functions                                                        * v1.0 *
// ===================================================================================
//
// 2023 by Stefan Wagner:   https://github.com/wagiminator

#pragma once
#include <stdint.h>
#include "gpio.h"
#include "config.h"

typedef enum class {
  HEX_MODE = 0x80,
  STRIP_LINE_ENDS = 0x40,
  AUTO_ACK = 0x20,
  DYNAMIC_PAYLOAD = 0x10
} options_t;

// NRF variables
extern __xdata uint8_t NRF_channel;             // channel (0x00 - 0x7F)
extern __xdata uint8_t NRF_speed;               // 0:250kbps, 1:1Mbps, 2:2Mbps
extern __xdata uint8_t NRF_tx_addr[];           // transmit address
extern __xdata uint8_t NRF_rx_addr[];           // receive address
extern __code uint8_t* NRF_STR[];               // speed strings
extern __xdata options_t options;

// NRF functions
void NRF_init(void);                            // init NRF
void NRF_configure(void);                       // configure NRF
uint8_t NRF_available(void);                    // check if data is available for reading
uint8_t NRF_readPayload(__xdata uint8_t *buf); // read payload into buffer, return length
void NRF_writePayload(__xdata uint8_t *buf, uint8_t len);  // send a data package (max length 32)
uint8_t NRF_readconfig(void);
uint8_t NRF_readstatus(void);
uint8_t NRF_readfifostatus(void);