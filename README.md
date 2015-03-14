# ipv6-tftp-bootloader
An IPV6 based tftp bootloader for networked microcontrollers (currently Microchip PIC33 with ENC28J60)

The purpose of this project is a create the minimal set of code that will:
* Initialise a Microchip PIC33 microcontroller and SPI attached ENC28J60 ethernet controller
* Get the MAC address from an I2C attached mc24aa00 EEPROM
* Respond to IPV6 neighbour solicitation messages (using the link-local EUI64 calculated from the MAC address)
* Respond to IPV6 ping requests
* Accept tftp write requests for a single binary file (the "user" code)
* Write the contents of the sent file to the program EEPROM of the microcontroller (and not overwrite the bootloader code)

There are two additional features
* Detect and count the holding of the reset button, beeping once per second (piezo attached to PWM output)
* Transfer execution to the "user" code with the button hold count (which could be 0)
* Execute the bootloader network code if the hold count is > HOLD_COUNT_TO_BOOTLOADER (maybe 3?)

The operation would normally be:
* Bootloader executes on reset
* Detects if reset button is held, counts seconds and beeps while held
* Once reset button released:
  * if hold count < 3, jump to "user" code, passing button hold count in
  * Otherwise execute bootloader network code
