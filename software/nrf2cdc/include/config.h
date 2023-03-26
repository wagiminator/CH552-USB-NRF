// ===================================================================================
// User Configurations for USB2NRF Stick
// ===================================================================================

#pragma once

// Pin definitions
#define PIN_CSN             P14       // NRF SPI slave select pin
#define PIN_MOSI            P15       // NRF SPI master out slave in (do not change)
#define PIN_MISO            P16       // NRF SPI master in slave out (do not change)
#define PIN_SCK             P17       // NRF SPI serial clock        (do not change)
#define PIN_LED             P30       // pin connected to builtin LED
#define PIN_IRQ             P31       // NRF interrupt pin
#define PIN_CE              P32       // NRF cable enable pin

// USB2NRF Settings
#define NRF_PAYLOAD         32        // NRF max payload (1-32)
#define NRF_CONFIG          0x0C      // CRC scheme, 0x08:8bit, 0x0C:16bit
#define FLASH_IDENT         0xA96C    // to identify if data flash was written
#define CMD_IDENT           '!'       // command string identifier

// USB device descriptor
#define USB_VENDOR_ID       0x16C0    // VID (shared www.voti.nl)
#define USB_PRODUCT_ID      0x27DD    // PID (shared CDC)
#define USB_DEVICE_VERSION  0x0100    // v1.0 (BCD-format)

// USB configuration descriptor
#define USB_MAX_POWER_mA    250       // max power in mA 

// USB descriptor strings
#define MANUFACTURER_STR    'w','a','g','i','m','i','n','a','t','o','r'
#define PRODUCT_STR         'U','S','B','2','N','R','F'
#define SERIAL_STR          'C','H','5','5','x','C','D','C'
#define INTERFACE_STR       'C','D','C','-','S','e','r','i','a','l'
