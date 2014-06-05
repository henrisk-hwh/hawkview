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

void* video_ret;
void* command_ret;
hawkview_handle hawkview;
int old_hv_cmd = 0;
int old_hv_status = 0;
struct stat old_stat;

extern int display_register(hawkview_handle* hawkview);
extern int capture_register(hawkview_handle* hawkview);

static void* hawkview_video(void* arg)
{
	int ret;
	
	while(1){
		if (old_hv_cmd != hawkview.cmd){
			hv_dbg("current video cmd is %d\n",hawkview.cmd);
			old_hv_cmd = hawkview.cmd;
		}
		if (old_hv_status != hawkview.status){
			hv_dbg("current video status is %d\n",hawkview.status);
			old_hv_status = hawkview.status;
		}		
		if(hawkview.status == VIDEO_EXIT) break;//todo , no test
		
		if(hawkview.status == VIDEO_WAIT) {
			if(hawkview.cmd == SET_CAP_SIZE || hawkview.cmd == START_STREAMMING){
				hv_dbg("reset video capture\n");
				ret = hawkview.capture.ops->cap_init((void*)&hawkview.capture);
				hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);
				hawkview.status = VIDEO_START;
			}

			continue;

		}
		if(hawkview.status == VIDEO_START) {

			if(hawkview.capture.ops->cap_frame){
				ret = hawkview.capture.ops->cap_frame((void*)(&hawkview.capture),hawkview.display.ops->disp_set_addr);
			}
			if(ret == 0) continue;
			
			if(ret == 2) {
				hv_dbg("video wait\n");
				hawkview.status = VIDEO_WAIT;
				continue;
			}

			if(ret == -1)
				return (void*)-1;
		}
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
int fetch_sub_cmd(const char* buf,char** cmd,int* cmd_num)
{
		int i = 0,j = 0;
		char size[20];
		memset(size,0,sizeof(size));
		if(buf[3] == ':'){
			hv_err("fetch sub cmd failed,the sub cmd format is invalidity, please make the cmd like:148:xx:xxx# ");
			return -1;
		}
		i = 4;		//the sub cmd start for 4th byte eg:148:1280x720#
		while(buf[i] != '#'){	//the sub cmd end by '#'
			
		}
}
int fetch_cmd()
{
	int ret = -1;
	FILE* fp = NULL;
	char buf[50];
	struct stat cmd_stat;
	ret = lstat("/data/camera/command",&cmd_stat);
	if(ret == -1){
		hv_err("can't lstat /data/camera/command,%s\n",strerror(errno));
		return ret;
	}
	memset(buf,0,sizeof(buf));
	if(cmd_stat.st_ctime != old_stat.st_ctime){
		old_stat.st_ctime = cmd_stat.st_ctime;
		fp = fopen("/data/camera/command","rwb+");
		if(fp){
			//todo flock;
			ret = fread(buf,50,50,fp);
			hv_dbg("read cmd string: %s\n",buf);
			//fwrite(cls,10,1,fp);
			fclose(fp);
			ret = atoi(buf);
		}
		hv_dbg("fetch -- cmd: %d\n",ret);
		if(ret == SET_CAP_SIZE){
			//eg: command string "148:1280x720#"
			int w,h;
			int i = 0,j = 0;
			char size[20];
			memset(size,0,sizeof(size));
			if(buf[3] == ':'){
				i = 4;
				while(buf[i] != 'x')	size[j++]=buf[i++];
				size[j+1] = '\0';
				w = atoi(size);
				
				memset(size,0,sizeof(size));
				j = 0;
				i++; 
				while(buf[i] != '#')	size[j++]=buf[i++];
				size[j+1] = '\0';
				h = atoi(size);
				hawkview.capture.set_w = w;
				hawkview.capture.set_h = h;
				hv_dbg("set size: %d x %d\n",w,h);
				
			}
		if(ret == SET_CAP_VIDEO){
			//eg: command string "147:video0:input0#"
			int i = 0,j = 0;
			char size[20];
			memset(size,0,sizeof(size));
			

		}
		}
	}
	return 	ret;

		
	
	
}
void alarm_command(int sig)
{
	int ret;
	
	ret = fetch_cmd();
	if(ret >= 0){
		//hawkview.state = ret;
		//hv_dbg("fetch cmd: %d\n",ret);
	}
	else
		goto set_alarm;
	
	if (ret == COMMAND_EXIT){
		hv_dbg("hawkview command thread exit!");
		alarm(0);
	
	}else if (ret == COMMAND_WAIT){

	}else if (ret == SAVE_FRAME){
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),SAVE_FRAME);

	}else if (ret == SET_CAP_SIZE){//command:148
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
		hawkview.cmd = SET_CAP_SIZE;

	}else if (ret == STOP_STREAMMING){//command:151
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
		hawkview.cmd = STOP_STREAMMING;
		
	}else if (ret == START_STREAMMING){//command:152
		//hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);
		hawkview.cmd = START_STREAMMING;

	}else if (ret == FULL_SCREEN){//command:200
		hawkview.display.ops->disp_send_command((void*)(&hawkview.display),FULL_SCREEN);

	}else if (ret == FULL_CAPTURE){//command:201
		hawkview.display.ops->disp_send_command((void*)(&hawkview.display),FULL_CAPTURE);
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
	memset(&old_stat,0,sizeof(struct stat));
	memset(&hawkview, 0, sizeof(hawkview_handle));
	memcpy(&hawkview,haw,sizeof(hawkview_handle));
	ret = display_register(&hawkview);
	if(ret == -1){
		hv_err("display_register failed\n");
		return -1;
	}

	ret = capture_register(&hawkview);
	if(ret == -1){
		hv_err("capture_register failed\n");		
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

	//start stream first
	hawkview.status = VIDEO_START;
	hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);

	signal(SIGALRM, alarm_command);
    alarm(1);

	start_video_thread((void*)&hawkview);
	//start_command_thread((void*)&hawkview);
	pthread_join(video_tid,&video_ret);
	//pthread_join(video_tid,&command_status);
	
	return 0;
}



void hawkview_quit()
{

}

