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

#define MAX_CMD_NUM 5

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
			if(hawkview.cmd == SET_CAP_SIZE || \
			   hawkview.cmd == START_STREAMMING ||\
			   hawkview.cmd == SET_CAP_VIDEO ||\
			   hawkview.cmd == SET_CAP_INFO){
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
int fetch_sub_cmd(const char* buf,char** cmd,int* cmd_num,int lenght)
{
		int i = 0,j = 0,n = 0;

		while(buf[i] != '#'){	//the sub cmd end by '#'
			while(buf[i] != 'x' && buf[i] != ':' && buf[i] != '#')	{
				*((char*)cmd + n*lenght + j++) = buf[i++];
				if(j > lenght) {
					hv_err("sub cmd over long\n");
					return -1;
				}
			}
			*((char*)cmd + n*lenght + j++) = '\0';
			n++;
			j = 0;
			if(buf[i] != '#'){
				i++;				
			}
			if(n > *cmd_num){
				hv_err("the max cmd num is %d\n",*cmd_num);
				return -1;
				
			}
		}
		*cmd_num = n;
		return 0;
}
int fetch_cmd()
{
	int ret = 0;
	int i;
	FILE* fp = NULL;
	char buf[50];
	char cmd[MAX_CMD_NUM][20];
	int n = MAX_CMD_NUM;
	
	struct stat cmd_stat;
	ret = lstat("/data/camera/command",&cmd_stat);
	if(ret == -1){
		hv_err("can't lstat /data/camera/command,%s\n",strerror(errno));
		return ret;
	}
	
	if(cmd_stat.st_ctime == old_stat.st_ctime) return 0;
	
	old_stat.st_ctime = cmd_stat.st_ctime;
	fp = fopen("/data/camera/command","rwb+");
	if(fp){
		//todo flock;
		memset(buf,0,sizeof(buf));
		memset(cmd,0,sizeof(cmd));
		ret = fread(buf,50,50,fp);
		//hv_dbg("read cmd string: %s\n",buf);
		fclose(fp);

	}
	ret = fetch_sub_cmd(buf,(char**)cmd,&n,20);
	for(i = 0;i < n;i++)
		hv_dbg("cmd %d: %s\n",i,cmd[i]);
	
	ret = atoi(cmd[0]);
	if(ret == SET_CAP_SIZE){
		//eg: command string "148:1280x720#"
		if (n < 2){hv_err("invalidity cmd num\n");return -1;}
		
		hawkview.capture.set_w = atoi(cmd[1]);
		hawkview.capture.set_h = atoi(cmd[2]);
		hv_dbg("set size: %d x %d\n",hawkview.capture.set_w,hawkview.capture.set_h);		
	}

	if(ret == SET_CAP_VIDEO){
		//eg: command string "147:0:1#"   video:0,s_input:1
		if (n < 2){hv_err("invalidity cmd num\n");return -1;}
		
		hawkview.capture.video_no = atoi(cmd[1]);
		hawkview.capture.subdev_id = atoi(cmd[2]);
	}
	if(ret == SET_CAP_INFO){
		if (n < 4){hv_err("invalidity cmd num\n");return -1;}
		
		hawkview.capture.video_no = atoi(cmd[1]);
		hawkview.capture.subdev_id = atoi(cmd[2]);
		hawkview.capture.set_w = atoi(cmd[3]);
		hawkview.capture.set_h = atoi(cmd[4]);

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


	}else if (ret == SET_CAP_INFO){//command:146
			hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
			hawkview.cmd = SET_CAP_INFO;

	}else if (ret == SET_CAP_VIDEO){//command:147
			hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
			hawkview.cmd = SET_CAP_VIDEO;

	}else if (ret == SET_CAP_SIZE){//command:148
		hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),STOP_STREAMMING);
		hawkview.cmd = SET_CAP_SIZE;

	}else if (ret == SAVE_IMAGE){//command:149
			hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),SAVE_IMAGE);

	}else if (ret == SAVE_FRAME){//command:150
			hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),SAVE_FRAME);

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
	#if 0
	if(hawkview.capture.ops->cap_init){
	hv_dbg("hawkview.capture.video_no: %d\n",hawkview.capture.video_no);
		//ret = hawkview.capture.ops->cap_init((void*)&hawkview.capture);
		if(ret == -1) {
			hv_err("capture init fail!\n");
			return -1;
		}else hv_dbg("capture init sucessfully\n");
	}
	else
		return -1;
	#endif
	//start stream first
	hawkview.status = VIDEO_WAIT;
	//hawkview.capture.ops->cap_send_command((void*)(&hawkview.capture),START_STREAMMING);

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

