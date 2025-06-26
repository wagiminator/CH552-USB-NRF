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
  if(options) {
    CDC_print ("Options:");
    if(options & HEX_MODE) CDC_print (" Hex mode,");
    if(options & STRIP_LINE_ENDS) CDC_print (" Strip line-ends,");
    if(options & AUTO_ACK) CDC_print (" Auto ACK,");
    if(options & DYNAMIC_PAYLOAD) CDC_print(" Dynamic payload");
    CDC_write('\n');
  }
  CDC_flush();
}

void NRF_interrupt()
{
  CDC_write('@');
}

// ===================================================================================
// Data Flash Implementation
// ===================================================================================

typedef struct {
  char    f_ident[2];
  uint8_t f_channel;
  uint8_t f_speed;
  char    f_tx_address[5];
  char    f_rx_address[5];
  uint8_t f_options;
} flash_t;

typedef enum {
  fo_ident = 0,
  fo_channel = 2,
  fo_speed = 3,
  fo_tx_address = 4,
  fo_rx_address = 9,
  fo_options = 15
} flash_offsets_t;

// FLASH write user settings
void FLASH_writeSettings(void) {
  uint8_t i;
  FLASH_update(fo_channel, NRF_channel);
  FLASH_update(fo_speed, NRF_speed);
  for(i=0; i<5; i++) {
    FLASH_update(fo_tx_address+i, NRF_tx_addr[i]);
    FLASH_update(fo_rx_address+i, NRF_rx_addr[i]);
  }
  FLASH_update(fo_options, options);
}

// FLASH read user settings; if FLASH values are invalid, write defaults
void FLASH_readSettings(void) {
  uint8_t i;
  uint16_t identifier = ((uint16_t)FLASH_read(1) << 8) | FLASH_read(0);
  if (identifier == FLASH_IDENT) {
    NRF_channel =  FLASH_read(fo_channel);
    NRF_speed   =  FLASH_read(fo_speed);
    for(i=0; i<5; i++) {
      NRF_tx_addr[i] = FLASH_read(fo_tx_address+i);
      NRF_rx_addr[i] = FLASH_read(fo_rx_address+i);
    }
    options = FLASH_read(fo_options);
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
    case 'o': for(char *ptr=&buffer[2]; *ptr != '\0'; ++ptr) {
                switch(*ptr) {
                  case 'l': options &= ~STRIP_LINE_ENDS; break;
                  case 'L': options |=  STRIP_LINE_ENDS; break;
                  case 'x': options &= ~HEX_MODE; break;
                  case 'X': options |=  HEX_MODE; break;
                  case 'a': options &= ~AUTO_ACK; break;
                  case 'A': options |=  AUTO_ACK; break;
                  case 'd': options &= ~DYNAMIC_PAYLOAD; break;
                  case 'D': options |=  DYNAMIC_PAYLOAD; break;
                  default: goto endoptions;
                }
              }
              endoptions:
              break;
    /*
    case '>': NRF_powerTX();  // manually switch to TX mode for 200uS
              DLY_us(200);
              NRF_powerRX();
              break;
    */
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
      char ch;
      while(buflen--) {
        ch = buffer[bufptr++];
        if(ch >= 0x20 && ch <= 0x7f) //printable
          CDC_write(ch);
        else if(ch == '\r' || ch == '\n')
          CDC_write(ch);
        else {
          CDC_write('\\');
          CDC_printByte(ch);
        }
      }
      if(ch != '\n')  // add a newline if we didn't end with one
        CDC_write('\n');
      CDC_flush();                                  // flush CDC
    }

    buflen = CDC_available();                       // get number of bytes in CDC IN
    uint8_t is_command;
    if(buflen) {                                    // something coming in via USB?
      bufptr = 0;                                   // reset output buffer pointer
      buffer[bufptr++] = CDC_read();                // read 1st byte to check if it is a command
      buflen--;
      is_command = (buffer[0] == CMD_IDENT); 
      if(!is_command && (options & HEX_MODE)) { // hex input
        // read the input as hex digits, and truncating as below. Note invalid characters=>\0
        buffer[0] = (hexDigit(buffer[0]) << 4) + hexDigit(CDC_read()); // redo the 1st byte
        buflen--;

        while(1) {
          // basically pull in pairs, but \r and \n must be handled specially
          if(buflen == 0) break; char ch1 = CDC_read(); buflen--;
          if(ch1 == '\r' || ch1 == '\n') {
            //CDC_write('#');
            if((options & STRIP_LINE_ENDS) == 0) {
              //CDC_write('^');
              buffer[bufptr++] = ch1; if(bufptr >= NRF_PAYLOAD) break;
            }
            continue;
          }

          if(buflen == 0) break; char ch2 = CDC_read(); buflen--;
          if(ch2 == '\r' || ch2 == '\n') {
            buffer[bufptr++] = hexDigit(ch1) << 4; if(bufptr >= NRF_PAYLOAD) break;
            if((options & STRIP_LINE_ENDS) == 0) {
              buffer[bufptr++] = ch2; if(bufptr >= NRF_PAYLOAD) break;
            }
            continue;
          }
          buffer[bufptr++] = (hexDigit(ch1) << 4) + hexDigit(ch2);
          if(bufptr >= NRF_PAYLOAD) break;
        }
      } else { // normal input
        if(buflen > NRF_PAYLOAD-1) buflen = NRF_PAYLOAD-1; // restrict length to max payload. First byte already buffered
        while(buflen--) {
          char ch = CDC_read(); // get data from CDC
          if(ch != '\r' && ch != '\n')            // output non-lineend character
            buffer[bufptr++] = ch;
          else if((options & STRIP_LINE_ENDS) == 0) // output lineend if we aren't stripping
            buffer[bufptr++] = ch;// get data from CDC
          }
      }

      if(is_command) {
        buffer[bufptr] = '\0';
        parse();           // is it a command? -> parse
      } else {                                        // not a command?
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
