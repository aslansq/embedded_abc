![all_builds](https://github.com/aslansq/embedded_abc/actions/workflows/c-cpp.yml/badge.svg)

# embedded_abc
This repository contains minimal demos of stm32f0discovery's peripherals with CMSIS.

![fun](./doc/boardFun.gif "fun")  

## Why?
<mark>It will help you to make connection between datasheet and physical hardware.</mark>
This is for educational purposes. Main purpose is learning simple communication interfaces. No usb/ethernet :), those are too big to manually set registers. If I have enough time, I ll put more peripherals other than communication.

|Goal|State|Demo|Description|
|-|-|-|-|
|Spi|&#x2713;|[03_spi_bit_bang](./03_spi_bit_bang/README.md)|Simplest way to push data with SPI|
|Uart|&#x2713;|[04_uart](./04_uart/README.md)|Pushing data with UART|
|I2C|&#x2713;|[05_i2c](./05_i2c/README.md)|Pushing data to I2C bus(note: slave is faked)|
|LIN|x||Need to find pull-up resistor|

|More|State|Demo|Description|
|-|-|-|-|
|Core Clock|&#x2713;|[00_core_clock](./00_core_clk/README.md)|Well to be able to do anything clocks needs to be setup. Core and bus clock configurations and core clock speed test|
|Systick|&#x2713;|[01_systick](./01_systick/README.md)|Systick 1 ms interrupt configuration|
|Gpio Out|&#x2713;|[02_gpio_out](./02_gpio_out/README.md)|Explained how board led configuration is done|
|Pwm|x|||

## Prerequisites
* Install(download and unzip path of your choice) [arm-none-eabi-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
* Create environment variable GCC_ARM_NONE_EABI_BIN_PATH which points to binaries.
* Install GNU make utility.
* Install CMake utility.
* Give execute permission files under shell. ```chmod +x shell/*.sh```

## Building
```
./shell/rebuild_all.sh
```
## Design Choices
* Every demo should be in a single main.c file
* Every demo should be compilable by it is own. No shared resources between demos.
* Only the lines that thought to be as tricky explained.
* All above choices made to keep it simple.

## References
See the references under doc folder.
* Reference manual of STM32F051R8T6 [rm0091](./doc/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
* Technical reference manual of Cortex-M0 [DDI0432C](./doc/DDI0432C_cortex_m0_r0p0_trm.pdf)
* Programming manual of STM32F051R8T6 [pm0215](./doc/pm0215-stm32f0-series-cortexm0-programming-manual-stmicroelectronics.pdf)
* Stm32f0discovery board schematic [MB1034](./doc/MB1034.pdf)
* STM32F051R8T6 datasheet [stm32f051r8](./doc/stm32f051r8.pdf)

## Abbreviations and Acronyms
|Short form|Description|
|-|-|
|APB|Advanced Peripheral Bus|
|HSI|High Speed Internal Oscillator|
|AHB|Advanced High Performance Bus|
|GPIO|General Purpose Input Output|
|NVIC|Nested Vector Interrupt Controller|
|SPI|Serial Peripheral Interface|
|UART|Universal Asynchronous Receiver/Transmitter|
