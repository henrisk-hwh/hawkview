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

#define HAWKVIEW_DBG 1

#if HAWKVIEW_DBG
#define hv_warn(x,arg...) printf("[hawkview_WARN]"x,##arg)
#endif
#define hv_dbg(x,arg...) printf("[hawkview_DBG]"x,##arg)
#define hv_err(x,arg...) printf("[hawkview_ERR]"x,##arg)

///////////////////////////////////////////////////////////////////////////////
//command
typedef enum _command
{
	COMMAND_UNUSED = 0,
		
	//for hawkview
	COMMAND_EXIT 		= 1,
	COMMAND_WAIT 		= 2,

	//video
	VIDEO_EXIT			= 100,
	VIDEO_WAIT			= 101,
	VIDEO_START 		= 102,
	//for video capture
	SAVE_FRAME 			= 150,
	STOP_STREAMMING 	= 151,
	START_STREAMMING	= 152,

	
	//for display	
	FULL_SCREEN 		= 200,
	FULL_CAPTURE 		= 201,
	
}command;

typedef enum _capture_status
{
	STREAM_ON,
	STREAM_OFF,
}capture_status;


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
	
	int cap_w;
	int cap_h;
	
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

	command state;
	
}hawkview_handle;

#endif

