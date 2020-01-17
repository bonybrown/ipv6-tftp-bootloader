# An IPV6, TFTP bootloader for networked microcontrollers

This project implements an IPV6 (UDP only) stack and Trivial FTP (tftp) server for use as a bootloader in embedded systems.

It currently supports platforms using a Microchip PIC33 CPU with SPI attached ENC28J60 network controller chip.

## The booloader:

* Detects and counts the holding of the reset button, beeping once per second (piezo attached to PWM output)
* Initialises a Microchip PIC33 microcontroller and SPI attached ENC28J60 ethernet controller
* Gets the MAC address from an I2C attached mc24aa00 EEPROM
* Responds to IPV6 neighbour solicitation messages (using the link-local EUI64 calculated from the MAC address)
* Responds to IPV6 ping requests
* Accepts tftp write requests for a single binary file (the "user" code)
* Writes the contents of the sent file to the program EEPROM of the microcontroller (without overwriting the bootloader code)


## Normal operation sequence:

* Bootloader executes on reset
* Detects if reset button is held
* While button is held, beep once per second and count number of beeps
* Once reset button released:
    * if hold count < 10
        * Detect if "user" code has not been uploaded/programmed and emit 3 quick beeps to indicate this, then halt.
        * Otherwise, jump to "user" code, making the button hold count available to the "user" code main() function
    * Otherwise, go into tftp server mode, ready to recieve uploaded "user" code if the hold count is equal to 10

## Building

This project is docker enabled - so it can create a suitable build environment into a docker container.

### Prerequisites

You will need at least GNU `make`. 

If you want to use docker for a build environment, you will need to install `docker.io`, and be a member of the `docker` group so that you can run docker commands. Ensure the `docker` service is running.

Otherwise, install the Microchip XC16 compiler.

### Building the docker build environment (one time)

`make docker` will build the `microchip/xc16` container

### Building the project

`make` will build the project.

If you have made the docker image, the build will execute in the docker container. Otherwise, the build runs in the local environment.

### Other make targets

* clean - cleans the project build
* prog - program the built .hex file to the CPU using the `pk2cmd` tool (local only)
* bin - converts the built .hex file to binary using `srec_cat` (docker enabled)

## Unit tests

Test exist under the /test directory.

### Prerequisites

The unit tests use the [NovaProva](http://novaprova.org/index.html) testing framework.

Coverage data requires `lcov` package.

### Make targets

From /test

* test ( default ) - builds and runs the unit tests
* coverage - runs the unit tests, collects coverage data and generates html output to /test/coverage/index.html

