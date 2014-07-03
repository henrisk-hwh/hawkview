#include "hawkview.h"

#define DELIVER_FRAMES_RATE 5

struct buffer
{
    void   *start;
    size_t length;
};


static struct buffer *buffers = NULL;
static int nbuffers = 0;
static int videofh = 0;
static int req_frame_num = 10;

int old_status = 0;
int old_vi_cmd = 0;

int get_framerate_status = 4;

static int is_x_sec(int sec,long long secs,long long usecs)
{
	static long long timestamp_old = 0;
	long long timestamp = usecs + secs* 1000000;
	if(timestamp_old == 0)
		timestamp_old = timestamp;
	if((timestamp - timestamp_old) > sec*1000000){
		timestamp_old = timestamp;
		return 1;
	}	
	return 0;
	
}
static float get_framerate(long long secs,long long usecs)
{
	static long long timestamp_old = 0;
	long long timestamp;
	long long rate;
	timestamp = usecs + secs* 1000000;
	rate = timestamp - timestamp_old;
	if(rate == 0 || timestamp_old == 0){
		timestamp_old = timestamp;
		return 0.0;
	}

	//hv_dbg("rate: %0.2f\n",1000000/(float)rate);
	timestamp_old = timestamp;
	return 1000000/(float)rate;

	#if 0
	static int rate = 0;
	
	float rate_tmp;
	if((timestamp - timestamp_old) > 1000000){
		timestamp_old = timestamp;
		rate_tmp = (float)rate/(float)1;
		rate = 0;
		return rate_tmp;
	}
	else rate++;
	#endif
	return 0.0;
	
}
static int write_file(char* file_path,char* string,int lenght )
{
	FILE* fp;
	fp = fopen(file_path,"wrb+");
	if(!fp) {
			hv_err("Open sync file error");
			return -1;
	}	
	
	//hv_dbg("%s: %s\n",file_path,string);
	if(fwrite(string,lenght,1,fp)){
			fclose(fp);
			return 0;
	}
	else{
			hv_err("Write file fail\n");
			fclose(fp);
			return -1;		
	}
}
static int make_exif_info(char* exif_str,char* name, struct isp_exif_attribute *exif,int w,int h)
{
	sprintf(exif_str,						\
				 "image name:      %s\n" 	\
				 "width:           = %d\n"	\
				 "height:          = %d\n"	\
				 "exp_time_num:    = %d\n"	\
				 "exp_time_den:    = %d\n"	\
				 "sht_speed_num:   = %d\n"	\
				 "sht_speed_den:   = %d\n"	\
				 "fnumber:         = %d\n"	\
				 "exp_bias:        = %d\n"	\
				 "foc_lenght:      = %d\n"	\
				 "iso_speed:       = %d\n"	\
				 "flash_fire:      = %d\n"	\
				 "brightness:      = %d\n#",	\
	name,			\
	w,						\
	h,						\
	exif->exposure_time.numerator,	\
	exif->exposure_time.denominator,\
	exif->shutter_speed.numerator,	\
	exif->shutter_speed.denominator,\
	exif->fnumber,					\
	exif->exposure_bias,			\
	exif->focal_length,				\
	exif->iso_speed,				\
	exif->flash_fire,				\
	exif->brightness);
	return 0;
}
static int set_exif_info(void* capture){
	char exif_str[1000];
	char file_path[50];
	capture_handle* cap = (capture_handle*)capture;
	struct isp_exif_attribute *exif = &(cap->picture.exif);
	memset(exif_str,0,sizeof(exif_str));
	sprintf(file_path,"/data/camera/%s.exif",cap->picture.path_name);
	exif = &(cap->picture.exif);
	make_exif_info(exif_str,cap->picture.path_name,exif,cap->cap_w,cap->cap_h);
#if 0
	//hv_dbg("file_path: %s\n",file_path);
	sprintf(exif_str,"image name:       %s\n" 	\
					 "width:            %d\n"	\
					 "height:           %d\n"	\
					 "exp_time_num:     %d\n"	\
					 "exp_time_den:     %d\n"	\
					 "sht_speed_num:    %d\n"	\
					 "sht_speed_den:    %d\n"	\
					 "fnumber:          %d\n"	\
					 "exp_bias:         %d\n"	\
					 "foc_lenght:       %d\n"	\
					 "iso_speed:        %d\n"	\
					 "flash_fire:       %d\n"	\
					 "brightness:       %d\n#",	\
		cap->picture.path_name,			\
		cap->cap_w,						\
		cap->cap_h,						\
		exif->exposure_time.numerator,	\
		exif->exposure_time.denominator,\
		exif->shutter_speed.numerator,	\
		exif->shutter_speed.denominator,\
		exif->fnumber,					\
		exif->exposure_bias,			\
		exif->focal_length,				\
		exif->iso_speed,				\
		exif->flash_fire,				\
		exif->brightness);
	hv_err("exif_str:\n %s\n",exif_str);
#endif
	return write_file(file_path,exif_str,sizeof(exif_str));;
}

static int set_cap_info(void* capture)
{
	int ret;
	char info[1000];
	char exif[1000];
	char file_path[20];
	capture_handle* cap = (capture_handle*)capture;
	memset(info,0,sizeof(info));
	strcpy(file_path,"dev/info");
	//sync string: sensor_type:save_status:framrate:capture_w:capture_h,sub_w,sub_h#
	sprintf(info,	\
				 "sensor_type:     = %s\n" 		\
				 "status:          = %d\n"		\
				 "framerate:       = %0.2f\n"	\
				 "subchanel_width  = %d\n"		\
				 "subchanel_height = %d\n"		\
				 "rotation         = %d\n\n",		\
			 (cap->sensor_type == 1)?"raw":"yuv",	\
			 cap->save_status,	\
			 cap->cap_fps,		\
			 cap->sub_w,		\
			 cap->sub_h,		\
			 cap->sub_rot);

	make_exif_info(exif,"none",&(cap->frame.exif),cap->cap_w,cap->cap_h);
	strcat(info,exif);
	hv_err("exif_str:\n %s\n",info);
	return write_file(file_path,info,sizeof(info));
}
static int set_sync_status(void* capture,int index)
{
	FILE* fp;
	char sync[30];
	char file_path[20];
	capture_handle* cap = (capture_handle*)capture;
	
	memset(sync,0,sizeof(sync));

	sprintf(sync,"%d", index);
	strcpy(file_path,"dev/sync");
	
	return write_file(file_path,sync,sizeof(sync));
	
}

static int save_frame(void* str,void* start,int w,int h,int format,int is_one_frame)
{
	FILE* fp; 
	int length;
	switch(format){
		case V4L2_PIX_FMT_NV12 :
			length = w*h*3>>1;
			break;
		default:
			hv_err("Unknow src data format\n");
			
	}
	if(is_one_frame)
			fp = fopen(str,"wrb+");		//save one frame data
	else			
			fp = fopen(str,"warb+");		//save more frames	//TODO: test
	if(!fp) {
			hv_err("Open file error\n");
			return -1;
	}
	if(fwrite(start,length,1,fp)){
			fclose(fp);
			return 0;
	}
	else {
			hv_err("Write file fail\n");
			fclose(fp);
			return -1;
	}
}

static int setVflipHflip(void* capture)
{
	int ret = -1;
	struct v4l2_control ctrl;
	capture_handle* cap = (capture_handle*)capture;
	hv_msg("set vflip and hflip\n");
	
	if(cap->sensor_type == 0){
		ctrl.id = V4L2_CID_VFLIP;
		ctrl.value = 1;
		ret = ioctl(videofh, VIDIOC_S_CTRL, &ctrl);
		if (ret != 0)
			hv_err("set vflip fail\n");

		ctrl.id = V4L2_CID_HFLIP;
		ctrl.value = 1;
		ret = ioctl(videofh, VIDIOC_S_CTRL, &ctrl);
		if (ret != 0)
			hv_err("set hflip fail\n");
	}else{
		ctrl.id = V4L2_CID_VFLIP_THUMB;
		ctrl.value = 1;
		ret = ioctl(videofh, VIDIOC_S_CTRL, &ctrl);
		if (ret != 0)
			hv_err("set vflip fail\n");

		ctrl.id = V4L2_CID_HFLIP_THUMB;
		ctrl.value = 1;
		ret = ioctl(videofh, VIDIOC_S_CTRL, &ctrl);
		if (ret != 0)
			hv_err("set hflip fail\n");

	}	
	return ret;
}
static int openDevice(void* capture)
{
	char dev_name[32];
	capture_handle* cap = (capture_handle*)capture;
	snprintf(dev_name, sizeof(dev_name), "/dev/video%d", cap->video_no);

	hv_msg("open %s\n", dev_name);
	if ((videofh = open(dev_name, O_RDWR,0)) < 0) {
		hv_err("can't open %s(%s)\n", dev_name, strerror(errno));
		return -1;
	}

	fcntl(videofh, F_SETFD, FD_CLOEXEC);
	return 0;
	
}
static int setVideoParams(void* capture)
{
	capture_handle* cap = (capture_handle*)capture;
	
	/* set input input index */
	struct v4l2_input inp;
	inp.index = cap->subdev_id;
	inp.type = V4L2_INPUT_TYPE_CAMERA;
	if (ioctl(videofh, VIDIOC_S_INPUT, &inp) == -1) {
		hv_err("VIDIOC_S_INPUT failed! s_input: %d\n",inp.index);
		inp.index = (inp.index == 1)?0:1;
		if (ioctl(videofh, VIDIOC_S_INPUT, &inp) == -1){
			hv_err("VIDIOC_S_INPUT failed! s_input: %d\n",inp.index);
			return -1;
		}
	}

	struct v4l2_streamparm parms;
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//parms.parm.capture.capturemode = V4L2_MODE_PREVIEW;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = cap->cap_fps;
	if (ioctl(videofh, VIDIOC_S_PARM, &parms) == -1) {
		hv_err("VIDIOC_S_PARM failed!\n");
		return -1;
	}

	/* set image format */
	struct v4l2_format fmt;
	struct v4l2_pix_format sub_fmt;
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&sub_fmt, 0, sizeof(struct v4l2_format));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = cap->set_w;
	fmt.fmt.pix.height      = cap->set_h;
	fmt.fmt.pix.pixelformat = cap->cap_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;

	if(cap->sensor_type == V4L2_SENSOR_TYPE_RAW){
		fmt.fmt.pix.subchannel = &sub_fmt;
		fmt.fmt.pix.subchannel->width = 640;
		fmt.fmt.pix.subchannel->height = 480;
		fmt.fmt.pix.subchannel->pixelformat = cap->cap_fmt;
		fmt.fmt.pix.subchannel->field = V4L2_FIELD_NONE;
		if(cap->sub_rot != 0)
			hv_msg("subchannel rot_angle: %d\n",cap->sub_rot);
			fmt.fmt.pix.subchannel->rot_angle = cap->sub_rot;		
	}else{
		if(cap->sub_rot != 0){			
			hv_msg("rot_angle: %d\n",cap->sub_rot);
			fmt.fmt.pix.rot_angle = cap->sub_rot;
		}
	}
	if (ioctl(videofh, VIDIOC_S_FMT, &fmt)<0) {
		hv_err("VIDIOC_S_FMT failed!\n");
		return -1;
	}
	hv_msg("the tried size is %dx%d,the supported size is %dx%d!\n",\
				cap->set_w,		\
				cap->set_h, 	\
				fmt.fmt.pix.width,	\
				fmt.fmt.pix.height);

	cap->cap_w = fmt.fmt.pix.width;
	cap->cap_h = fmt.fmt.pix.height;
	
	if(cap->sensor_type == V4L2_SENSOR_TYPE_RAW){
		hv_msg("the subchannel size is %dx%d\n",fmt.fmt.pix.subchannel->width,fmt.fmt.pix.subchannel->height);
		cap->sub_w = fmt.fmt.pix.subchannel->width;
		cap->sub_h = fmt.fmt.pix.subchannel->height;
	}
	return 0;
	
}
static int reqBuffers(void* capture)
{
	int i;
	struct v4l2_requestbuffers req;
	capture_handle* cap = (capture_handle*)capture;
	
	/* request buffer */
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count  = req_frame_num;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(videofh, VIDIOC_REQBUFS, &req)<0){
		hv_err("VIDIOC_REQBUFS failed\n");
		return -1;
	}

	buffers = calloc(req.count, sizeof(struct buffer));
	for (nbuffers = 0; nbuffers < req.count; nbuffers++) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = nbuffers;

		if (ioctl(videofh, VIDIOC_QUERYBUF, &buf) == -1) {
			hv_err("VIDIOC_QUERYBUF error\n");
			return -2;	//goto release buffer
		}

		buffers[nbuffers].start  = mmap(NULL, buf.length, 
										PROT_READ | PROT_WRITE, \
										MAP_SHARED, videofh, \
										buf.m.offset);
		buffers[nbuffers].length = buf.length;
		if (buffers[nbuffers].start == MAP_FAILED) {
			hv_err("mmap failed\n");
			return -2;	//goto release buffer
		}
		hv_dbg("index: %d, mem: %x, len: %x, offset: %x\n",
				nbuffers,(int)buffers[nbuffers].start,buf.length,buf.m.offset);

		if (ioctl(videofh, VIDIOC_QBUF, &buf) == -1) {
			hv_err("VIDIOC_QBUF error\n");
			return -3;//goto umap
		}		
	}
	
	
}
static int getExifInfo(struct isp_exif_attribute *exif)
{
	int ret = -1;
	if (videofh == NULL)
	{
		return 0xFF000000;
	}
	ret = ioctl(videofh, VIDIOC_ISP_EXIF_REQ, exif);
	return ret;
}

static int getSensorType(void* capture)
{
	int ret = -1;
	struct v4l2_control ctrl;
	struct v4l2_queryctrl qc_ctrl;

	capture_handle* cap = (capture_handle*)capture;

	if (videofh == NULL)
	{
		return 0xFF000000;
	}

	ctrl.id = V4L2_CID_SENSOR_TYPE;
	qc_ctrl.id = V4L2_CID_SENSOR_TYPE;

	if (-1 == ioctl (videofh, VIDIOC_QUERYCTRL, &qc_ctrl))
	{
		hv_err("query sensor type ctrl failed");
		return -1;
	}
	ret = ioctl(videofh, VIDIOC_G_CTRL, &ctrl);
	cap->sensor_type = ctrl.value;
	return ret;
}

static int capture_init(void* capture)
{

	int ret;
	int i;
	
	capture_handle* cap = (capture_handle*)capture;
	cap->save_status = OFF;
	cap->status = OFF;
	cap->cmd = COMMAND_UNUSED;
	get_framerate_status = 4;

	ret = openDevice(capture);
	if(ret == -1)
		goto open_err;

	getSensorType(capture);
	hv_msg("get sensor type: %d\n",cap->sensor_type);
	
	ret = setVideoParams(capture);
	if(ret == -1)
		goto err;

	if(cap->sub_rot == 180)
		setVflipHflip(capture);
	ret = reqBuffers(capture);
	if(ret == -1)
		goto err;
	else if(ret == -2)
		goto buffer_rel;
	else if(ret == -3)
		goto unmap;

	return 0;

unmap:
	for (i = 0; i < nbuffers; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}
buffer_rel:
	free(buffers);	
err:
	close(videofh);
open_err:
	return -1;
}

static int capture_frame(void* capture,int (*set_disp_addr)(int,int,unsigned int*))
{
	capture_handle* cap = (capture_handle*)capture;
	int ret;
	int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;	

	fd_set fds;
	struct timeval tv;
	
	//used for cammand and status debug
	if (old_vi_cmd != cap->cmd){
		hv_dbg("capture frame command is %d\n",cap->cmd);
		old_vi_cmd = (int)cap->cmd;
	}
	if(old_status != cap->status){
		hv_dbg("capture frame status is %d\n",cap->status);
		old_status = cap->status;
	}

	if(cap->status == OFF && cap->cmd == START_STREAMMING){
		hv_dbg("capture start streaming\n");
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(videofh, VIDIOC_STREAMON, &type) == -1) {
			hv_err("VIDIOC_STREAMON error\n");
			goto quit;
		}
		cap->status = ON;
		cap->cmd = COMMAND_UNUSED;
		return 0;
	}
	
	if(cap->status == ON && cap->cmd == STOP_STREAMMING){
		hv_dbg("capture stop streaming\n");
		if(-1==ioctl(videofh, VIDIOC_STREAMOFF, &type)){
			hv_err("VIDIOC_STREAMOFF error!\n");
			goto quit;
		}
		cap->status = OFF;
		cap->cmd = COMMAND_UNUSED;
		capture_quit(capture);
		return 2;
	}

	if(cap->status == OFF) return 0;
	FD_ZERO(&fds);
	FD_SET(videofh, &fds);

	tv.tv_sec  = 2;
	tv.tv_usec = 0;
	ret = select(videofh + 1, &fds, NULL, NULL, &tv);
	//hv_dbg("select video ret: %d\n",ret);
	if (ret == -1) {
		if (errno == EINTR) {
			return 0;
		}
		hv_err("select error\n");
		goto stream_off;
	}
	else if (ret == 0) {
		hv_err("select timeout\n");
		return 0;
	}

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(videofh, VIDIOC_DQBUF, &buf);
	if (ret == -1) {
		hv_err("VIDIOC_DQBUF failed!\n");		
		goto stream_off;		
	}

	//get display addr
	unsigned int addrPhyY;
	int target_h;
	int target_w;
	addrPhyY = buf.m.offset;
	if(cap->sensor_type == V4L2_SENSOR_TYPE_RAW){
		addrPhyY = addrPhyY + ALIGN_4K(ALIGN_16B(cap->cap_w) * cap->cap_h * 3 >> 1);	
		target_h = cap->sub_h;
		target_w = cap->sub_w;
	}else{
		target_h = cap->cap_h;
		target_w = cap->cap_w;
	}
	if(cap->sub_rot == 90 || cap->sub_rot == 270){
		int tmp;
		addrPhyY = addrPhyY + ALIGN_4K(ALIGN_16B(target_w) * target_h * 3 >> 1);		
		tmp = target_h;
		target_h = target_w;
		target_w = tmp;
		
	}
	// set disp buffer
	if (set_disp_addr){		
		set_disp_addr(target_w,target_h,&addrPhyY);
	}
	
	float framerate;		
	framerate = get_framerate((long long)(buf.timestamp.tv_sec),(long long)(buf.timestamp.tv_usec));
	if(framerate > 1.0){
		cap->cap_fps = framerate;
		//hv_dbg("framerate: %0.2ffps\n",cap->cap_fps);
	}
	if(is_x_sec(1,(long long)(buf.timestamp.tv_sec),(long long)(buf.timestamp.tv_usec))){
		getExifInfo(&(cap->frame.exif));
		set_cap_info((void*)cap);
	}

	//set capture info
	if((cap->cmd == SAVE_FRAME 		&& cap->save_status == OFF)	||	\
	   (cap->cmd == STOP_SAVE_FRAME && cap->save_status == ON)){
	   cap->save_status = (cap->save_status + 1)%2; 
		set_cap_info((void*)cap);
	   cap->save_status = (cap->save_status + 1)%2; 
	}
	if(cap->cmd == STOP_SAVE_FRAME && cap->save_status == ON)
		cap->save_status = OFF;
	
	if(cap->cmd == SAVE_FRAME || cap->save_status == ON ) {
		static int index = 0;
		static int interval = 0;
		int tmp_interval = 5;
		
		if(cap->cmd == SAVE_FRAME){
			cap->save_status = ON;
			cap->cmd = COMMAND_UNUSED;		
		}
		if((cap->show_rate != 0) && (cap->show_rate < cap->cap_fps) && (cap->show_rate > 0))
			tmp_interval = cap->cap_fps/cap->show_rate;
		if(interval-- == 0){
 			char name[30];
			int sub_start;
 			snprintf(name, sizeof(name), "dev/frame_%d", index);
			if(cap->sensor_type == V4L2_SENSOR_TYPE_RAW){					
				sub_start = (unsigned int)(buffers[buf.index].start) + ALIGN_4K(ALIGN_16B(cap->cap_w) * cap->cap_h * 3 >> 1);
				if(cap->sub_rot == 90 || cap->sub_rot == 270){
					//sub_start = (unsigned int)(sub_start) + ALIGN_4K(ALIGN_16B(cap->sub_w) * cap->sub_h * 3 >> 1);
				}
				ret = save_frame(name,								\
							   (void*)sub_start,					\
							   cap->sub_w,cap->sub_h,cap->cap_fmt,	\
							   1);
			}
			else{
				sub_start = (unsigned int)(buffers[buf.index].start);
				if(cap->sub_rot == 90 || cap->sub_rot == 270){
					//sub_start = sub_start + ALIGN_4K(ALIGN_16B(cap->cap_w) * cap->cap_h * 3 >> 1);
				}				
				ret = save_frame(name,								\
							   (void*)sub_start,					\
							   cap->cap_w,cap->cap_h,cap->cap_fmt,	\
							   1);
			}
			if(ret == -1){
				hv_err("save frame failed!\n");
			}			
			set_sync_status((void*)cap,index);
			index++ ;
			if(index > 20) index = 0;
			interval = tmp_interval - 1;
			

		}
		//cap->cmd = COMMAND_UNUSED;
	}
	if(cap->cmd == SAVE_IMAGE ) {
		char image_name[30];
		memset(image_name,0,sizeof(image_name));
		sprintf(image_name,"/data/camera/%s", cap->picture.path_name);		
		hv_dbg("image_name: %s\n",image_name);		

		ret = getExifInfo(&(cap->picture.exif));
		if(ret == 0)
			set_exif_info(capture);
		hv_dbg("--------set_exif_info end\n");
		ret = save_frame(image_name,				\
					  (void*)(buffers[buf.index].start),	\
					  cap->cap_w,cap->cap_h,cap->cap_fmt,	\
					  1);
		if(ret == -1)
			hv_err("save image failed!\n");		
		
		cap->cmd = COMMAND_UNUSED;
	}	
	ret = ioctl(videofh, VIDIOC_QBUF, &buf);
	if (ret == -1) {
		hv_err("VIDIOC_DQBUF failed!\n");		
		goto stream_off;
	}
	return 0;
	
stream_off:
	hv_err("err stream off\n");
	ioctl(videofh, VIDIOC_STREAMOFF, &type);
quit:
	capture_quit(capture);
	return -1;

	
}

int capture_quit(void *capture)
{
	int i;
	enum v4l2_buf_type type;
	hv_msg("capture quit!\n");
	for (i = 0; i < nbuffers; i++) {
		hv_dbg("ummap index: %d, mem: %x, len: %x\n",
				i,(int)buffers[i].start,buffers[i].length);		
		munmap(buffers[i].start, buffers[i].length);
	}
	free(buffers);
	close(videofh);
	return 0;
}

int capture_command(void* capture,command state)
{
	capture_handle* cap = (capture_handle*)capture;
	//todo: add metux
	cap->cmd = state;
	return 0;
}


static struct cap_ops ops = {
	.cap_init  = capture_init,
	.cap_frame = capture_frame,
	.cap_quit  = capture_quit,
	.cap_send_command = capture_command,

};
static const capture_handle sun9iw1p1_cap = {
	.ops = &ops,
};

int capture_register(hawkview_handle* hawkview)
{
	if (hawkview == NULL){
		hv_err("hawkview handle is NULL,sunxi9iw1p1 capture register fail\n");
		return -1;
	}
	hawkview->capture.ops = &ops;
	hv_msg("sunxi9iw1p1 capture register sucessfully!\n");
	return 0;
}

