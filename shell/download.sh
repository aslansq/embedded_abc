#!/bin/bash
# This script is used to download demoElf to board
# Usage: ./download.sh <demoElf binary file>
demoElf="$1"

thisPath=$(realpath "$0")
thisDirPath=$(dirname "$thisPath")
source "${thisDirPath}/util.sh"

if [ ! -f "${demoElf}" ]
then
    echoerr "demoElf does not exist ${demoElf}"
    ungracefulExit
fi

if [ -f ${STM32_Programmer_CLI_PATH}/STM32_Programmer_CLI ]
then
    echo STM32_Programmer_CLI --connect port=SWD mode=UR reset=HWrst --download ${demoElf}
    ${STM32_Programmer_CLI_PATH}/STM32_Programmer_CLI --connect port=SWD mode=UR reset=HWrst --download ${demoElf}
else
    echoerr "Could not find flashing tool STM32_Programmer_CLI"
    ungracefulExit
fi