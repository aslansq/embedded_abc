#!/bin/bash
# This script is used to rebuild demo and download to board
# Usage: ./rebuild_download.sh <demo folder path>
demoPath="$1"
demoElf=${demoPath}/build/demo.elf

thisPath=$(realpath "$0")
thisDirPath=$(dirname "$thisPath")
source "${thisDirPath}/util.sh"

if [ ! -d "${demoPath}" ]
then
    echoerr "demo path does not exist ${demoPath}"
    ungracefulExit
fi

${thisDirPath}/rebuild.sh ${demoPath}
if [ $? != 0 ]
then
    echoerr "rebuild failed ${demoPath}"
    ungracefulExit
fi

if [ ! -f "${demoElf}" ]
then
    echoerr "demo elf does not exist ${demoElf}"
    ungracefulExit
fi

${thisDirPath}/download.sh ${demoElf}

if [ $? != 0 ]
then
    echoerr "download failed ${demoElf}"
    ungracefulExit
fi

echo SUCCESS!!
