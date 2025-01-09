#!/bin/bash

demos="\
00_core_clk \
01_systick \
02_gpio_out \
03_spi_bit_bang \
04_uart \
05_i2c \
06_gpio_in \
07_pwm"

getMacros()
{
    pass=""
}

echoerr() { echo "$@" 1>&2; }

ungracefulExit()
{
    echoerr
    echoerr ERROR!! $0
    exit 1
}

echoWTab()
{
    while IFS= read -r line; do
    echo -e "\t$line"
    done <<< "$@"
}