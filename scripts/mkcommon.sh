#!/bin/bash
#
# scripts/mkcommon.sh
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
HAWKVIEW_TOP_DIR=`pwd`
[ -f .buildconfig ] && . .buildconfig

if [ "x$1" = "xconfig" ] ; then
	. ${HAWKVIEW_TOP_DIR}/scripts/mksetup.sh
	make
	exit $?
	
elif [ "x$1" = "xclean" ] ; then
	make distclean
	exit $?	

elif [ $# -eq 0 ] ; then
	if [ ! -f .buildconfig ] ; then
		. ${HAWKVIEW_TOP_DIR}/scripts/mksetup.sh
		make distclean
	fi
	make
	exit $?	
fi
