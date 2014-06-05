#include "hawkview.h"


struct buffer
{
    void   *start;
    size_t length;
};


static struct buffer *buffers = NULL;
static int nbuffers = 0;
static int videofh = 0;
static int req_frame_num = 7;

int old_status = 0;
int old_vi_cmd = 0;

static int save_frame(void* str,void* start,int w,int h,int format,int is_one_frame)
{
	FILE* fd; 
	int length;
	switch(format){
		case V4L2_PIX_FMT_NV12 :
			length = w*h*3>>1;
			break;
		default:
			hv_err("Unknow src data format\n");
			
	}
	if(is_one_frame)
			fd = fopen(str,"wrb+");		//save one frame data
	else			
			fd = fopen(str,"warb+");		//save more frames
	if(!fd) {
			hv_err("Open file error");
			return -1;
	}
	if(fwrite(start,length,1,fd)){
			hv_dbg("start addr: %x\n",start);
			hv_dbg("%d x %d\n",w,h);
			hv_dbg("length: %d\n",length);
			hv_dbg("Write file successfully\n");
			fclose(fd);
			return 0;
			}
	else {
			hv_err("Write file fail\n");
			fclose(fd);
			return -1;

	}
}
static int capture_init(void* capture)
{
	int i;
	char dev_name[32];
	struct v4l2_input inp;
	struct v4l2_format fmt;
	struct v4l2_streamparm parms;
	struct v4l2_requestbuffers req;
	capture_handle* cap = (capture_handle*)capture;

	cap->status = STREAM_OFF;
	cap->cmd = COMMAND_UNUSED;
	
	//hv_dbg("cap->video_no:%d\n",cap->video_no);
	snprintf(dev_name, sizeof(dev_name), "/dev/video%d", cap->video_no);

	hv_dbg("open %s\n", dev_name);
	if ((videofh = open(dev_name, O_RDWR,0)) < 0) {
		hv_err("can't open %s(%s)\n", dev_name, strerror(errno));
		goto open_err;
	}

	fcntl(videofh, F_SETFD, FD_CLOEXEC);

	/* set input input index */
	inp.index = cap->subdev_id;
	inp.type = V4L2_INPUT_TYPE_CAMERA;
	if (ioctl(videofh, VIDIOC_S_INPUT, &inp) == -1) {
		hv_err("VIDIOC_S_INPUT failed!\n");
		goto err;
	}

	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//parms.parm.capture.capturemode = V4L2_MODE_PREVIEW;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = cap->cap_fps;
	if (ioctl(videofh, VIDIOC_S_PARM, &parms) == -1) {
		hv_err("VIDIOC_S_PARM failed!\n");
		goto err;
	}

	/* set image format */
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = cap->set_w;
	fmt.fmt.pix.height      = cap->set_h;
	fmt.fmt.pix.pixelformat = cap->cap_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	if (ioctl(videofh, VIDIOC_S_FMT, &fmt)<0) {
		hv_err("VIDIOC_S_FMT failed!\n");
		goto err;
	}
	hv_dbg("the tried size is %dx%d,the supported size is %dx%d!\n",\
				cap->set_w,		\
				cap->set_h, 	\
				fmt.fmt.pix.width,	\
				fmt.fmt.pix.height);

	cap->cap_w = fmt.fmt.pix.width;
	cap->cap_h = fmt.fmt.pix.height;
	
	/* request buffer */
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count  = req_frame_num;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(videofh, VIDIOC_REQBUFS, &req)<0){
		hv_err("VIDIOC_REQBUFS failed\n");
		goto err;
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
			goto buffer_rel; 
		}

		buffers[nbuffers].start  = mmap(NULL, buf.length, 
		PROT_READ | PROT_WRITE, MAP_SHARED, videofh, buf.m.offset);
		buffers[nbuffers].length = buf.length;
		if (buffers[nbuffers].start == MAP_FAILED) {
			hv_err("mmap failed\n");
			goto buffer_rel;
		}
		hv_dbg("index: %d, mem: %x, len: %x, offset: %x\n",
				nbuffers,(int)buffers[nbuffers].start,buf.length,buf.m.offset);

		if (ioctl(videofh, VIDIOC_QBUF, &buf) == -1) {
			hv_err("VIDIOC_QBUF error\n");
			goto unmap;
		}		
	}

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

	if(cap->status == STREAM_OFF && cap->cmd == START_STREAMMING){
		hv_dbg("capture start streaming\n");
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(videofh, VIDIOC_STREAMON, &type) == -1) {
			hv_err("VIDIOC_STREAMON error\n");
			goto quit;
		}
		cap->status = STREAM_ON;
		cap->cmd = COMMAND_UNUSED;
		return 0;
	}
	
	if(cap->status == STREAM_ON && cap->cmd == STOP_STREAMMING){
		hv_dbg("capture stop streaming\n");
		if(-1==ioctl(videofh, VIDIOC_STREAMOFF, &type)){
			hv_err("VIDIOC_STREAMOFF error!\n");
			goto quit;
		}
		cap->status = STREAM_OFF;
		cap->cmd = COMMAND_UNUSED;
		capture_quit(capture);
		return 2;
	}

	if(cap->status == STREAM_OFF) return 0;
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
	
	// set disp buffer
	if (set_disp_addr)
		set_disp_addr(cap->cap_w,cap->cap_h,&buf.m.offset);
	if(cap->cmd == SAVE_FRAME) {
		if(save_frame("data/camera/nv12",(void*)(buffers[buf.index].start),cap->cap_w,cap->cap_h,cap->cap_fmt,1) == -1){
			hv_err("save frame failed!\n");
		}
		
		cap->cmd = COMMAND_UNUSED;
	}
	if(cap->cmd == SAVE_IMAGE) {
		if(save_frame("data/camera/image",(void*)(buffers[buf.index].start),cap->cap_w,cap->cap_h,cap->cap_fmt,1) == -1){
			hv_err("save image failed!\n");
		}
		
		cap->cmd = COMMAND_UNUSED;
	}	
	ret = ioctl(videofh, VIDIOC_QBUF, &buf);
	if (ret == -1) {
		hv_err("VIDIOC_DQBUF failed!");		
		goto stream_off;
		
	}
	return 0;
	
stream_off:
	hv_dbg("err stream off\n");
	ioctl(videofh, VIDIOC_STREAMOFF, &type);
quit:
	capture_quit(capture);
	return -1;

	
}

int capture_quit(void *capture)
{
	int i;
	enum v4l2_buf_type type;
	hv_dbg("capture quit!\n");
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
	hv_dbg("sunxi9iw1p1 capture register sucessfully!\n");
	return 0;
}

