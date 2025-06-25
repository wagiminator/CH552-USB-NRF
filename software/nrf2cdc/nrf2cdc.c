// ===================================================================================
// Project:   NRF2CDC for nRF24L01+ 2.4GHz Transceiver USB Stick based on CH55x
// Version:   v1.2
// Year:      2023
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// NRF2CDC is a simple development tool for wireless applications based on the
// nRF24L01+ 2.4GHz transceiver module. It provides a serial interface for
// communication with the module via USB CDC.
//
// References:
// -----------
// - Blinkinlabs: https://github.com/Blinkinlabs/ch554_sdcc
// - Deqing Sun: https://github.com/DeqingSun/ch55xduino
// - Ralph Doncaster: https://github.com/nerdralph/ch554_sdcc
// - WCH Nanjing Qinheng Microelectronics: http://wch.cn
// - ATtiny814 NRF2USB: https://github.com/wagiminator/ATtiny814-NRF2USB
//
// Compilation Instructions:
// -------------------------
// - Chip:  CH551, CH552 or CH554
// - Clock: 16 MHz internal
// - Adjust the firmware parameters in src/config.h if necessary.
// - Make sure SDCC toolchain and Python3 with PyUSB is installed.
// - Press BOOT button on the board and keep it pressed while connecting it via USB
//   with your PC.
// - Run 'make flash' immediatly afterwards.
// - To compile the firmware using the Arduino IDE, follow the instructions in the 
//   .ino file.
//
// Operating Instructions:
// -----------------------
// Plug the device into a USB port, it should be detected as a CDC device. Open a 
// serial monitor, BAUD rate doesn't matter. Enter the text to be sent, terminated
// with a Newline (NL or '\ n'). A string that begins with an exclamation mark ('!')
// is recognized as a command. The command is given by the letter following the
// exclamation mark. Command arguments are appended as bytes in 2-digit hexadecimal
// directly after the command. The following commands can be used to set the NRF:
//
// cmd  description       example         example description
// -----------------------------------------------------------------------------------
//  c   set channel       !c2A            set channel to 0x2A (0x00 - 0x7F)
//  t   set TX address    !t7B271F1F1F    addresses are 5 bytes, LSB first
//  r   set RX address    !r41C355AA55    addresses are 5 bytes, LSB first
//  s   set speed         !s02            data rate (00:250kbps, 01:1Mbps, 02:2Mbps)
//
// Enter just the exclamation mark ('!') for the actual NRF settings to be printed
// in the serial monitor. The selected settings are saved in the data flash and are
// retained even after a restart.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include "src/config.h"                   // user configurations
#include "src/system.h"                   // system functions
#include "src/gpio.h"                     // GPIO functions
#include "src/delay.h"                    // delay functions
#include "src/flash.h"                    // data flash functions
#include "src/usb_cdc.h"                  // USB-CDC serial functions
#include "src/nrf24l01.h"                 // nRF24L01+ functions

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

void NRF_interrupt(void);
void NRF_ISR(void) __interrupt(INT_NO_GPIO) {
  NRF_interrupt();
}

// Global variables
__xdata uint8_t buffer[NRF_PAYLOAD];      // rx/tx buffer
uint8_t hex_mode = 0;

// ===================================================================================
// Print Functions and String Conversions
// ===================================================================================

// Convert byte nibble into hex character and print via CDC
void CDC_printNibble(uint8_t nibble) {
  (nibble <= 9) ? (nibble += '0') : (nibble += ('A' - 10));
  CDC_write(nibble);
}

// Convert byte into hex string and print via CDC
void CDC_printByte(uint8_t value) {
  CDC_printNibble (value >> 4);
  CDC_printNibble (value & 0x0F);
}

// Convert an array of bytes into hex string and print via CDC
void CDC_printBytes(uint8_t *ptr, uint8_t len) {
  while(len--) CDC_printByte(*ptr++);
}

// Convert character representing a hex nibble into 4-bit value
uint8_t hexDigit(uint8_t c) {
  if     ((c >= '0') && (c <= '9')) return(c - '0');
  else if((c >= 'a') && (c <= 'f')) return(c - 'a' + 10);
  else if((c >= 'A') && (c <= 'F')) return(c - 'A' + 10); 
  return 0;
}

// Convert string containing a hex byte into 8-bit value
uint8_t hexByte(uint8_t *ptr) {
  return((hexDigit(*ptr++) << 4) + hexDigit(*ptr));
}

// Convert string containing 5 hex bytes into address array
void hexAddress(uint8_t *sptr, uint8_t *aptr) {
  uint8_t i;
  for(i=5; i; i--) {
    *aptr++ = hexByte(sptr);
    sptr += 2;
  }
}

// Print the current NRF settings via CDC
void CDC_printSettings(void) {
  uint8_t cfg_reg = NRF_readconfig();
  uint8_t status_reg = NRF_readstatus();
  CDC_println("# nRF24L01+ Configuration:");
  CDC_print  ("# RF channel: "); CDC_printByte (NRF_channel);    CDC_write('\n');
  CDC_print  ("# TX address: "); CDC_printBytes(NRF_tx_addr, 5); CDC_write('\n');
  CDC_print  ("# RX address: "); CDC_printBytes(NRF_rx_addr, 5); CDC_write('\n');
  CDC_print  ("# Data rate:  "); CDC_print(NRF_STR[NRF_speed]);  CDC_println("bps");
  CDC_print ("Config register: "); CDC_printByte(cfg_reg); CDC_write('\n');
  CDC_print ("Status register: "); CDC_printByte(status_reg); CDC_write('\n');
  CDC_print ("FIFO Status register: "); CDC_printByte(NRF_readfifostatus()); CDC_write('\n');
  if(hex_mode) CDC_println  ("Hex Mode active");
  CDC_flush();
}

void NRF_interrupt()
{
  CDC_write('@');
}

// ===================================================================================
// Data Flash Implementation
// ===================================================================================

// FLASH write user settings
void FLASH_writeSettings(void) {
  uint8_t i;
  FLASH_update(2, NRF_channel);
  FLASH_update(3, NRF_speed);
  for(i=0; i<5; i++) {
    FLASH_update(4+i, NRF_tx_addr[i]);
    FLASH_update(9+i, NRF_rx_addr[i]);
  }
}

// FLASH read user settings; if FLASH values are invalid, write defaults
void FLASH_readSettings(void) {
  uint8_t i;
  uint16_t identifier = ((uint16_t)FLASH_read(1) << 8) | FLASH_read(0);
  if (identifier == FLASH_IDENT) {
    NRF_channel =  FLASH_read(2);
    NRF_speed   =  FLASH_read(3);
    for(i=0; i<5; i++) {
      NRF_tx_addr[i] = FLASH_read(4+i);
      NRF_rx_addr[i] = FLASH_read(9+i);
    }
  }
  else {
    FLASH_update(0, (uint8_t)FLASH_IDENT);
    FLASH_update(1, (uint8_t)(FLASH_IDENT >> 8));
    FLASH_writeSettings();
  }
}

// ===================================================================================
// Command Parser
// ===================================================================================
void parse(void) {
  uint8_t cmd = buffer[1];                          // read the command
  switch(cmd) {                                     // what command?
    case 'c': NRF_channel = hexByte(buffer + 2) & 0x7F;
              break;
    case 't': hexAddress(buffer + 2, NRF_tx_addr);
              break;
    case 'r': hexAddress(buffer + 2, NRF_rx_addr);
              break;
    case 's': NRF_speed = hexByte(buffer + 2);
              if(NRF_speed > 2) NRF_speed = 2;
              break;
    case 'm': if(buffer[2]=='x') hex_mode = 1; else hex_mode=0;
              break;
    case '>': NRF_powerTX();
              DLY_us(200);
              NRF_powerRX();
              break;
    default:  break;
  }
  NRF_configure();                                  // reconfigure the NRF
  CDC_printSettings();                              // print settings via CDC
  FLASH_writeSettings();                            // update settings in data flash
}

// ===================================================================================
// Main Function
// ===================================================================================
void main(void) {
  // Variables
  uint8_t buflen;                                   // data length in buffer
  uint8_t bufptr;                                   // buffer pointer

  // Setup
  CLK_config();                                     // configure system clock
  DLY_ms(5);                                        // wait for clock to settle
  FLASH_readSettings();                             // read user settings from flash
  CDC_init();                                       // init USB CDC
  NRF_init();                                       // init nRF24L01+
  WDT_start();                                      // start watchdog timer

  // Loop
  while(1) {
    if(NRF_available()) {                           // something coming in via NRF?
      PIN_low(PIN_LED);                             // switch on LED
      bufptr = 0;                                   // reset buffer pointer
      buflen = NRF_readPayload(buffer);             // read payload into buffer
      CDC_print("Read 0x"); CDC_printByte(buflen); CDC_write('\n'); 

      // escape unprintable
      while(buflen--) {
        char ch = buffer[bufptr++];
        if(ch >= 0x20 && ch <= 0x7f) //printable
          CDC_write(ch);
        else if(ch == '\r' || ch == '\n')
          CDC_write(ch);
        else {
          CDC_write('\\');
          CDC_printByte(ch);
        }
      }
      CDC_flush();                                  // flush CDC
    }

    buflen = CDC_available();                       // get number of bytes in CDC IN
    uint8_t is_command;
    if(buflen) {                                    // something coming in via USB?
      bufptr = 0;                                   // reset output buffer pointer
      buffer[bufptr++] = CDC_read();                // read 1st byte to check if it is a command
      buflen--;
      is_command = (buffer[0] == CMD_IDENT); 
      if(!is_command && hex_mode) {
        // read the input as hex digits, and truncating as below. Note invalid characters=>\0
        buffer[0] = (hexDigit(buffer[0]) << 4) + hexDigit(CDC_read()); // redo the 1st byte
        buflen--;

        int outlen = buflen / 2;
        if(outlen > NRF_PAYLOAD-1) outlen = NRF_PAYLOAD - 1;  // first byte is already buffered
        while(1) {
          if(buflen == 0) break; char ch1 = CDC_read(); buflen--;
          if(ch1 == '\r' || ch1 == '\n') {
            buffer[bufptr++] = ch1; if(bufptr >= NRF_PAYLOAD) break;
            continue;
          }
          
          if(buflen == 0) break; char ch2 = CDC_read(); buflen--;
          if(ch2 == '\r' || ch2 == '\n') {
            buffer[bufptr++] = hexDigit(ch1) << 4; if(bufptr >= NRF_PAYLOAD) break;
            buffer[bufptr++] = ch2; if(bufptr >= NRF_PAYLOAD) break;
            continue;
          }
          buffer[bufptr++] = (hexDigit(ch1) << 4) + hexDigit(ch2);
          if(bufptr >= NRF_PAYLOAD) break;
        }
      } else {
        if(buflen > NRF_PAYLOAD-1) buflen = NRF_PAYLOAD-1; // restrict length to max payload. First byte already buffered
        while(buflen--) buffer[bufptr++] = CDC_read();// get data from CDC
      }

      if(is_command) parse();           // is it a command? -> parse
      else {                                        // not a command?
        PIN_low(PIN_LED);                           // switch on LED
        //CDC_write('>');
        NRF_writePayload(buffer, bufptr);           // send the buffer via NRF
        CDC_print("Sent 0x"); CDC_printByte(bufptr); CDC_write('\n'); 
        CDC_flush();
      }
    }

    PIN_high(PIN_LED);                              // switch off LED
    WDT_reset();                                    // reset watchdog
  }
}
