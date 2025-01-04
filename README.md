![all_builds](https://github.com/aslansq/embedded_abc/actions/workflows/c-cpp.yml/badge.svg)

# embedded_abc
This repository contains minimal demos of stm32f0discovery's peripherals with CMSIS.

![fun](./doc/boardFun.gif "fun")  

## Why?
<mark>It will help you to make connection between datasheet and physical hardware.</mark>
This is for educational purposes.

## Design Choices
* Every demo should be in a single main.c file
* Every demo should be compilible by it is own. No shared resources between demos.
* Only the lines that thought to be as tricky explained.
* All above choices made to keep it simple.

## Demos
|Demo| Description|
|---|---|
|[00_core_clock](./00_core_clk/README.md)|Core and bus clock configurations and core clock speed test|
|[01_systick](./01_systick/README.md)|Systick 1 ms interrupt configuration|
|[02_gpio_out](./02_gpio_out/README.md)|Explained how board led configuration is done|

## References
See the references under doc folder.
* Reference manual of STM32F051R8T6 [rm0091](./doc/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
* Technical reference manual of Cortex-M0 [DDI0432C](./doc/DDI0432C_cortex_m0_r0p0_trm.pdf)
* Programming manual of STM32F051R8T6 [pm0215](./doc/pm0215-stm32f0-series-cortexm0-programming-manual-stmicroelectronics.pdf)

## Abbreviations and Acronyms
|Short form|Description|
|-|-|
|APB|Advanced Peripheral Bus|
|HSI|High Speed Internal Oscillator|
|AHB|Advanced High Performance Bus|
|GPIO|General Purpose Input Output|
|NVIC|Nested Vector Interrupt Controller|