This demo was made with ESP-IDF version 5.0.1

What it does?

It turns on a relay for a second, then turns it off, then turns on the next relay, then turns it off. USER_LED1 is also turned on when relay 2 and relay 4 are on. If you press and hold the button BUT1, the LED sequnece gets paused.

How to compile and download?

First make sure that the board is properly recognized and lisited in your operating system. Make sure to force the program/bootloader mode by holding button BUT1 then press RST1 and release BUT1. Install drivers if needed. 

Under "Windows" this should be listed as "Composite Device" and you should see "USB JTAG/serial debug unit (interface ...) (COMX)" in the "Windows Device Manager".

Set the chip we are about to program to ESP32-C6:

	idf.py set-target esp32c6

If you want to change anything about the application from the menu:
	
	idf.py menuconfig

Build the project:
	
	idf.py build

In order to flash the board you need to set it in bootloader mode (hold the button BUT on power-up/reset then release it).

	idf.py flash -p COMX

To run the program you need to restart the board.

If you have trouble with compilation use the prebuilt file "ESP32-C6-EVB_Blink.bin".
