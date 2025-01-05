#!/bin/bash

demos="\
00_core_clk \
01_systick \
02_gpio_out \
03_spi_bit_bang \
04_uart"

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