# scripts/mksetup.sh
#
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
HAWKVIEW_TOP_DIR=`pwd`
function select_platform()
{
    local cnt=0
    local choice
		local call=$1

    printf "All available platform:\n"
    for platformdir in ${HAWKVIEW_TOP_DIR}/platform/* ; do
        platforms[$cnt]=`basename $platformdir`
        printf "%4d. %s\n" $cnt ${platforms[$cnt]}
        ((cnt+=1))
    done

    while true ; do
        read -p "Choice: " choice
        if [ -z "${choice}" ] ; then
            continue
        fi

        if [ -z "${choice//[0-9]/}" ] ; then
            if [ $choice -ge 0 -a $choice -lt $cnt ] ; then
                export HAWKVIEW_PLATFORM="${platforms[$choice]}"
                echo "export HAWKVIEW_PLATFORM=${platforms[$choice]}" >> .buildconfig
                break
            fi
        fi
        printf "Invalid input ...\n"
    done
}

function mksetup()
{
    rm -f .buildconfig
    printf "\n"
    printf "Welcome to mkscript setup progress\n"

    select_platform
}

# Setup all variables in setup.
mksetup

