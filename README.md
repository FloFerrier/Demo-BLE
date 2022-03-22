# Demo-BLE
[![Build Status](https://app.travis-ci.com/FloFerrier/Demo-BLE.svg?branch=master)](https://app.travis-ci.com/FloFerrier/Demo-BLE)

This project contains source code for embedded target and python script.

- Embedded Target: NUCLEO-F446RE
- Embedded Plugin: Board with RN4871 (module BLE)

## Embedded Environment
### Start the project
Get submodules HAL with libopencm3 and OS with FreeRTOS:
```bash
$ git submodule update --init
$ git submodule update --remote --merge
```

### Make commands
 - ```make release```: compilation in release mode
 - ```make debug```: compilation in debug mode
 - ```make clean```:
 - ```make mrproper```:
 - ```make flash```: flash program on the board
 - ```make gdb```: start program on debugging mode

### Debugging with logs
A console debug is enabled for viewing log through UART.
```bash
$ sudo apt install picocom
$ sudo picocom -b 115200 /dev/ttyUSB0
```

## Python script
