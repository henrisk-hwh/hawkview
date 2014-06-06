/*
 * \file        hawkview.h
 * \brief       
 *
 * \version     1.0.0
 * \date        2014-5-22
 * \author      Henrisk <heweihong@allwinnertech.com>
 * 
 * Copyright (c) 2014 Allwinner Technology. All Rights Reserved.
 *
 */

///////////////////////////////////////////////////////////////////////////////
//debug setting
#ifndef __HAWKVIEW_H__
#define __HAWKVIEW_H__
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <signal.h>

#include <pthread.h>

#include "command.h"
#define HAWKVIEW_DBG 1

#if HAWKVIEW_DBG
#define hv_warn(x,arg...) printf("[hawkview_warn]"x,##arg)
#define hv_dbg(x,arg...) printf("[hawkview_dbg]"x,##arg)
#else
#define hv_warn(x,arg...)
#define hv_dbg(x,arg...) 
#endif
#define hv_err(x,arg...) printf("[hawkview_err]"x,##arg)
#define hv_msg(x,arg...) printf("[hawkview_msg]"x,##arg)

///////////////////////////////////////////////////////////////////////////////


typedef enum _capture_status
{
	STREAM_ON,
	STREAM_OFF,
}capture_status;

typedef enum _video_status
{
	//for video thread
	VIDEO_EXIT			= 100,
	VIDEO_WAIT			= 101,
	VIDEO_START 		= 102,

}video_status;

///////////////////////////////////////////////////////////////////////////////
//display

struct disp_ops
{
	int (*disp_init)(void *);
	int (*disp_set_addr)(int , int , unsigned int *);
	int (*disp_quit)(void *);
	int (*disp_send_command)(void *,command);
};

typedef struct _display
{
	//display
	int x;
	int y;
	int disp_w;
	int disp_h;

	int input_w;
	int input_h;
	
	struct disp_ops *ops;
	
	command state;
}display_handle;

///////////////////////////////////////////////////////////////////////////////
//capture
struct cap_ops
{
	int (*cap_init)(void*);
	int (*cap_frame)(void*,int (*)(int,int,unsigned int*));
	int (*cap_quit)(void*);
	int (*cap_send_command)(void*,command);
};

typedef struct _capture
{
	int video_no;		// /dev/video device
	int subdev_id;		// v4l2 subdevices id

	int set_w;			//request target capture size
	int set_h;		

	int cap_w;			//supported capture size
	int cap_h;

	int sub_w;			//sub channel size
	int sub_h;
	
	int cap_fmt;	//capture format
	int cap_fps;	//capture framerate
	
	struct cap_ops *ops;
	
	command cmd;

	//for status
	capture_status status;

}capture_handle;



///////////////////////////////////////////////////////////////////////////////
//command
typedef struct _hawkview
{

	capture_handle capture;

	display_handle display;

	command cmd;
	video_status status;
}hawkview_handle;

#endif

