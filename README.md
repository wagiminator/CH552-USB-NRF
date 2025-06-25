# USB2NRF - USB Controlled 2.4GHz Transceiver
NRF2USB is a simple development tool designed specifically for wireless applications that utilize the low-cost nRF24L01+ 2.4GHz transceiver module. The integrated CH55x microcontroller provides a USB interface for communication with the module. The versatility of this tool makes it an ideal choice for a wide range of wireless applications.

Depending on the firmware, it can be used, for example, to transfer serial data wirelessly between two PCs (USB CDC - Communications Device Class) or as a receiver for wireless keyboards, mice or joysticks (USB HID - Human Interface Device). It is also possible to receive sensor data or control remote actuators. This makes it ideal for IoT (Internet of Things) applications where data collected from sensors needs to be transmitted wirelessly to a central device for analysis.

![USB2NRF_pic1.jpg](https://raw.githubusercontent.com/wagiminator/CH552-USB-NRF/main/documentation/USB2NRF_pic1.jpg)

# Hardware
## Schematic
![USB2NRF_wiring.png](https://raw.githubusercontent.com/wagiminator/CH552-USB-NRF/main/documentation/USB2NRF_wiring.png)

## nRF24L01+ 2.4GHz Transceiver Module
The nRF24L01+ is a highly integrated, ultra-low power (ULP) 2Mbps RF transceiver IC for the 2.4GHz ISM (Industrial, Scientific and Medical) band. It is designed to be used as a wireless communication module in a variety of applications, such as home automation, wireless gaming, and the Internet of Things (IoT). The module is equipped with an SPI interface, which makes it simple to connect to a variety of microcontrollers, such as the Arduino, Raspberry Pi, and others.

## CH552G 8-bit USB Device Microcontroller
The CH552G is a low-cost, enhanced E8051 core microcontroller compatible with the MCS51 instruction set. It has an integrated USB 2.0 controller with full-speed data transfer (12 Mbit/s) and supports up to 64 byte data packets with integrated FIFO and direct memory access (DMA). The CH552G has a factory built-in bootloader so firmware can be uploaded directly via USB without the need for an additional programming device.

![USB2NRF_pic2.jpg](https://raw.githubusercontent.com/wagiminator/CH552-USB-NRF/main/documentation/USB2NRF_pic2.jpg)
![USB2NRF_pic3.jpg](https://raw.githubusercontent.com/wagiminator/CH552-USB-NRF/main/documentation/USB2NRF_pic3.jpg)

# Software
## NRF to CDC
This firmware provides a serial interface for communication with the module via USB CDC. It can be used to transfer serial data wirelessly between two PCs, receive data from a remote sensor or trasmit commands to an actuator.

### Operating Instructions:
Open a serial monitor and set the correct serial port (BAUD rate doesn't matter). Enter the text to be sent, terminated with a Newline (NL or '\ n'). A string that begins with an exclamation mark ('!') is recognized as a command. The command is given by the letter following the exclamation mark. Command arguments are appended as bytes in 2-digit hexadecimal directly after the command. The following commands can be used to set the NRF:

|Command|Description|Example|Example Description|
|-|:-|:-|:-|
|c|set channel|!c2A|set channel to 0x2A (0x00 - 0x7F)|
|t|set TX address|!t7B271F1F1F|addresses are 5 bytes, LSB first|
|r|set RX address|!r41C355AA55|addresses are 5 bytes, LSB first|
|s|set speed|!s02|data rate (00:250kbps, 01:1Mbps, 02:2Mbps)|
|m|set input mode|!mx| iff mode is 'x', then non-command input will be treated as hex digits and converted to binary before sending.|

Enter just the exclamation mark ('!') for the actual NRF settings to be printed in the serial monitor. The selected settings are saved in the data flash and are retained even after a restart.

## NRF to HID
- in development...

# Compiling and Installing Firmware
## Preparing the CH55x Bootloader
### Installing Drivers for the CH55x Bootloader
On Linux you do not need to install a driver. However, by default Linux will not expose enough permission to upload your code with the USB bootloader. In order to fix this, open a terminal and run the following commands:

```
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="4348", ATTR{idProduct}=="55e0", MODE="666"' | sudo tee /etc/udev/rules.d/99-ch55x.rules
sudo service udev restart
```

For Windows, you need the [CH372 driver](http://www.wch-ic.com/downloads/CH372DRV_EXE.html). Alternatively, you can also use the [Zadig Tool](https://zadig.akeo.ie/) to install the correct driver. Here, click "Options" and "List All Devices" to select the USB module, and then install the libusb-win32 driver. To do this, the board must be connected and the CH55x must be in bootloader mode.

### Entering CH55x Bootloader Mode
A brand new chip starts automatically in bootloader mode as soon as it is connected to the PC via USB. Once firmware has been uploaded, the bootloader must be started manually for new uploads. To do this, the board must first be disconnected from the USB port and all voltage sources. Now press the BOOT button and keep it pressed while reconnecting the board to the USB port of your PC. The chip now starts again in bootloader mode, the BOOT button can be released and new firmware can be uploaded within the next couple of seconds.

## Compiling and Uploading using the makefile
### Installing SDCC Toolchain for CH55x
Install the [SDCC Compiler](https://sdcc.sourceforge.net/). In order for the programming tool to work, Python3 must be installed on your system. To do this, follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition [pyusb](https://github.com/pyusb/pyusb) must be installed. On Linux (Debian-based), all of this can be done with the following commands:

```
sudo apt install build-essential sdcc python3 python3-pip
sudo pip install pyusb
```

### Compiling and Uploading Firmware
- Open a terminal.
- Navigate to the folder with the makefile. 
- Connect the board and make sure the CH55x is in bootloader mode. 
- Run ```make flash``` to compile and upload the firmware. 
- If you don't want to compile the firmware yourself, you can also upload the precompiled binary. To do this, just run ```python3 ./tools/chprog.py firmware.bin```.

## Compiling and Uploading using the Arduino IDE
### Installing the Arduino IDE and CH55xduino
Install the [Arduino IDE](https://www.arduino.cc/en/software) if you haven't already. Install the [CH55xduino](https://github.com/DeqingSun/ch55xduino) package by following the instructions on the website.

### Compiling and Uploading Firmware
- Open the .ino file in the Arduino IDE.
- Go to **Tools -> Board -> CH55x Boards** and select **CH552 Board**.
- Go to **Tools** and choose the following board options:
  - **Clock Source:**   16 MHz (internal)
  - **Upload Method:**  USB
  - **USB Settings:**   USER CODE /w 266B USB RAM
- Connect the board and make sure the CH55x is in bootloader mode. 
- Click **Upload**.

## Installing CDC driver
On Linux you do not need to install a CDC driver. On Windows you will need the [Zadig tool](https://zadig.akeo.ie/) to install the correct driver for the CDC device. Click "Options" and "List All Devices" to select the CDC device, then install the CDC driver.

# References, Links and Notes
1. [EasyEDA Design Files](https://oshwlab.com/wagiminator)
2. [CH551/552 Datasheet](http://www.wch-ic.com/downloads/CH552DS1_PDF.html)
3. [SDCC Compiler](https://sdcc.sourceforge.net/)
4. [nRF24L01+ Datasheet](https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf)
5. [ATtiny814 USB2NRF](https://github.com/wagiminator/ATtiny814-NRF2USB)
6. [CH32X033 USB2NRF](https://github.com/wagiminator/CH32X033-USB-NRF)
7. [nRF24L01+ on Aliexpress](http://aliexpress.com/wholesale?SearchText=nrf24l01+plus+smd)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
