/*
 * \file        hawkview.c
 * \brief       
 *
 * \version     1.0.0
 * \date        2014-5-22
 * \author      Henrisk <heweihong@allwinnertech.com>
 * 
 * Copyright (c) 2014 Allwinner Technology. All Rights Reserved.
 *
 */
#include <pthread.h>

#include "hawkview.h"

static pthread_t video_tid;
static pthread_t command_tid;

void* video_status;
void* command_status;
hawkview_handle hawkview;

extern int display_register(hawkview_handle* hawkview);
extern int capture_register(hawkview_handle* hawkview);

static void* hawkview_video(void* arg)
{
	int ret;
	
	while(1){
		hv_dbg("current video state is %d\n",hawkview.state);
		if(hawkview.state == VIDEO_EXIT) break;
		
		if(hawkview.state == VIDEO_WAIT) return 0;

		if(hawkview.state == START_STREAMMING){
			hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);
			hawkview.state = VIDEO_START;
			continue;
		} 

		if(hawkview.capture.ops->cap_frame){
			ret = hawkview.capture.ops->cap_frame((void*)(&hawkview.capture),hawkview.display.ops->disp_set_addr);
		}
		if(ret == 0) {
			//hv_dbg("continue\n");
			continue;
		}

		if(ret == -1)
			return (void*)-1;

	}

	if(hawkview.capture.ops->cap_quit)
		hawkview.capture.ops->cap_quit((void*)(&hawkview.capture));
		
	return (void*)0;

}




static void* hawkview_command(void* arg)
{
	while(1){

	}
	return 0;
}

int fetch_cmd()
{
	int ret = -1;
	FILE* fp;
	char buf[10];
	fp = fopen("/data/camera/command","rwb");
	if(fp){
		ret = fread(buf,10,10,fp);
		
		fclose(fp);
		ret = atoi(buf);
	}
	hv_dbg("read cmd: %d\n",ret);
	return 	ret;

		
	
	
}
void alarm_command(int sig)
{
	int ret;
	hv_dbg("hawkview_command state %d\n",hawkview.state);
	
	ret = fetch_cmd();
	if(ret != -1){
		hawkview.state = ret;
	}
	else
		goto set_alarm;
	
	if (hawkview.state == COMMAND_EXIT){
		hv_dbg("hawkview command thread exit!");
		alarm(0);
	
	}else if (hawkview.state == COMMAND_WAIT){

	}else if (hawkview.state == SAVE_FRAME){
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),SAVE_FRAME);
		hawkview.state == COMMAND_WAIT;
	}else if (hawkview.state == STOP_STREAMMING){
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
		hawkview.state == COMMAND_WAIT;
	}else if (hawkview.state == START_STREAMMING){
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);
		hawkview.state == COMMAND_WAIT;
	}else if (hawkview.state == FULL_SCREEN){
		hawkview.display.ops->disp_send_command((void*)(&hawkview.display),FULL_SCREEN);
		hawkview.state == COMMAND_WAIT;
	}else if (hawkview.state == FULL_CAPTURE){
		hawkview.display.ops->disp_send_command((void*)(&hawkview.display),FULL_CAPTURE);
		hawkview.state == COMMAND_WAIT;
	}

set_alarm:	
    alarm(1);

	
    return;
}

int start_video_thread(hawkview_handle* hawkview)
{
	int ret;
	video_tid = 0;
	ret = pthread_create(&video_tid, NULL, hawkview_video, (void *)hawkview);
	hv_dbg("video pthread_create ret:%d\n",ret);
    if ( ret == -1) {
       	hv_err("camera: can't create video thread(%s)\n", strerror(errno));
    	return -1;
	}
	return ret;
}

int start_command_thread(hawkview_handle* hawkview)
{
	int ret;
	command_tid = 0;
	ret = pthread_create(&command_tid, NULL, hawkview_command, (void *)hawkview);
	hv_dbg("command pthread_create ret:%d\n",ret);
    if ( ret == -1) {
       	hv_err("camera: can't create command thread(%s)\n", strerror(errno));
    	return -1;
	}
	return ret;
	
}

int hawkview_init(hawkview_handle* haw)
{
	int ret;
	
	memset(&hawkview, 0, sizeof(hawkview_handle));
	memcpy(&hawkview,haw,sizeof(hawkview_handle));
	hv_err("hawkview video no: %d\n",hawkview.capture.video_no);
	hv_err("video no: %d\n",haw->capture.video_no);
	ret = display_register(&hawkview);
	if(ret == -1){
		return -1;
	}

	ret = capture_register(&hawkview);
	if(ret == -1){
		return -1;
	}
	
	if(hawkview.display.ops->disp_init){
		ret = hawkview.display.ops->disp_init((void*)&hawkview.display);
		if(ret == -1) {
			hv_err("display init fail!\n");
			return -1;
		}else hv_dbg("display init sucessfully\n");
	}
	if(hawkview.capture.ops->cap_init){
	hv_dbg("hawkview.capture.video_no: %d\n",hawkview.capture.video_no);
		ret = hawkview.capture.ops->cap_init((void*)&hawkview.capture);
		if(ret == -1) {
			hv_err("capture init fail!\n");
			return -1;
		}else hv_dbg("capture init sucessfully\n");
	}
	else
		return -1;

	hawkview.capture.state = START_STREAMMING;

	signal(SIGALRM, alarm_command);
    alarm(1);

	start_video_thread((void*)&hawkview);
	//start_command_thread((void*)&hawkview);
	pthread_join(video_tid,&video_status);
	//pthread_join(video_tid,&command_status);
	
	return 0;
}



void hawkview_quit()
{

}

