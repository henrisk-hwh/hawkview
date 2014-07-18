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
HAWKVIEW_MK_DIR=${HAWKVIEW_TOP_DIR}/scripts
HAWKVIEW_RELEASE_DIR=${HAWKVIEW_TOP_DIR}/release

[ -f .buildconfig ] && . .buildconfig

if [ "x$1" = "xconfig" ] ; then
	. ${HAWKVIEW_MK_DIR}/mksetup.sh
	make
	exit $?
	
elif [ "x$1" = "xclean" ] ; then
	make distclean
	exit $?	

elif [ "x$1" = "xrelease" ] ; then
	echo "====now release the hawkview tool===="
	echo "platform: ${HAWKVIEW_PLATFORM}"
	echo "cleaning......"
	make distclean
	echo "rebulid the project......"
	make -s
	DATE=`date "+(%Y%m%d%H%M%S)"`
	HAWKVIEW_RELEASE_NAME=hawkview_${HAWKVIEW_PLATFORM}${DATE}
	mkdir -p ${HAWKVIEW_RELEASE_DIR}/${HAWKVIEW_PLATFORM}	
	
	echo "path:     ./release/${HAWKVIEW_PLATFORM}/"
	echo "target:   ${HAWKVIEW_RELEASE_NAME}"
	
	cp -r hawkview ${HAWKVIEW_RELEASE_DIR}/${HAWKVIEW_PLATFORM}/${HAWKVIEW_RELEASE_NAME}
	echo "=================end================="
	exit $?	

elif [ $# -eq 0 ] ; then
	if [ ! -f .buildconfig ] ; then
		. ${HAWKVIEW_MK_DIR}/mksetup.sh
		make distclean
	fi
	make
	exit $?	
fi
