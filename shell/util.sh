#!/bin/bash

demos="\
00_core_clk"

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