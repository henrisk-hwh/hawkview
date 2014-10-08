#include "sun8iw6p1_display.h"
#include "hawkview.h"

struct test_layer_info
{
	int screen_id;
	int layer_id;
	int mem_id;
	disp_layer_config layer_config;
	int addr_map;
	int width,height;//screen size
	int dispfh;//device node handle
	int fh;//picture resource file handle
	int mem;
	int clear;//is clear layer
	char filename[32];
        int full_screen;
};

struct disp_screen
{
	int x;
	int y;
	int w;
	int h;
};
struct test_layer_info test_info;

static struct disp_screen get_disp_screen(int w1, int h1, int w2, int h2)
{
	struct disp_screen screen;
	float r1,r2,rmin;
	r1 = (float)w1/(float)w2;
	r2 = (float)h1/(float)h2;
	if(r1 < r2){
		screen.w = w2*r1;
		screen.h = h2*r1;
	}else{
		screen.w = w2*r2;
		screen.h = h2*r2;
	}

	screen.x = (w1 - screen.w)/2;	
	screen.y = (h1 - screen.h)/2;
	//hv_dbg("w1:%d, h1:%d, w2:%d, h2:%d\n",w1,h1,w2,h2);
	//hv_dbg("x:%d, y:%d, w:%d, h:%d\n",screen.x,screen.x,screen.w,screen.h);
	return screen;
}

static int disp_init(void* display)
{
	display_handle* disp = (display_handle*)display;
	unsigned int arg[6];
	int screen_id = 0;
	int layer_id = 0;

	memset(&test_info, 0, sizeof(struct test_layer_info));

	//open device /dev/disp
	if((test_info.dispfh = open("/dev/disp",O_RDWR)) == -1) {
		hv_err("open display device fail!\n");
		return -1;
	}
	
	//get current output type
	disp_output_type output_type;
	for (screen_id = 0;screen_id < 3;screen_id++){
		arg[0] = screen_id;
		output_type = (disp_output_type)ioctl(test_info.dispfh, DISP_GET_OUTPUT_TYPE, (void*)arg);
		if(output_type != DISP_OUTPUT_TYPE_NONE){
			hv_dbg("the output type: %d\n",screen_id);
			break;
		}
	}

	test_info.full_screen = 1;

	test_info.screen_id = screen_id; //0 for lcd ,1 for hdmi
	test_info.layer_config.channel = 0;
	test_info.layer_config.layer_id = layer_id;
	test_info.layer_config.info.zorder = 1;
	//test_info.layer_config.info.ck_enable        = 0;
	test_info.layer_config.info.alpha_mode       = 1; //global alpha
	test_info.layer_config.info.alpha_value      = 0xff;
	//test_info.layer_config.info.pipe             = 0;
	test_info.width = ioctl(test_info.dispfh,DISP_GET_SCN_WIDTH,(void*)arg);	//get screen width and height
	test_info.height = ioctl(test_info.dispfh,DISP_GET_SCN_HEIGHT,(void*)arg);
        hv_dbg("screen w: %d, h: %d\n",test_info.width,test_info.height);
	/* crop setting */
	//test_info.layer_config.info.fb.crop.x     = x;
	//test_info.layer_config.info.fb.crop.y     = y;
	//test_info.layer_config.info.fb.crop.width = (unsigned long long)width << 32;
	//.layer_config.info.fb.crop.height= (unsigned long long)height << 32;
	//display window of the screen


	//mode
	test_info.layer_config.info.mode = LAYER_MODE_BUFFER;

	//data format
	test_info.layer_config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;

	return 0;
}
static int disp_quit(void* display)
{
	//display_handle* disp = (display_handle*)display;
	int ret;
	unsigned int arg[6];
	test_info.layer_config.enable = 0;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 0;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		hv_err("fail to set layer info\n");

	close(test_info.dispfh);
	memset(&test_info, 0, sizeof(struct test_layer_info));

	return 0;
}
static int disp_set_addr(int width, int height, unsigned int *addr)

{
	unsigned int arg[6];
	int ret;
	disp_layer_config layer_config;
/*	
	//source frame size
	test_info.layer_info.fb.size.width = width;
	test_info.layer_info.fb.size.height = height;

	test_info.layer_info.fb.src_win.width = width;
	test_info.layer_info.fb.src_win.height = height;
*/
	//display window of the screen
	if (test_info.full_screen == 0 && 	\
		width < test_info.width && 		\
		height < test_info.height){
		test_info.layer_config.info.screen_win.x = (test_info.width - width)/2;
		test_info.layer_config.info.screen_win.y = (test_info.height - height)/2;
		test_info.layer_config.info.screen_win.width    = width;
		test_info.layer_config.info.screen_win.height   = height;
	}
	else{
		struct disp_screen screen;
		screen = get_disp_screen(test_info.width,test_info.height,width,height);
		test_info.layer_config.info.screen_win.x = screen.x;
		test_info.layer_config.info.screen_win.y = screen.y;
		test_info.layer_config.info.screen_win.width    = screen.w;
		test_info.layer_config.info.screen_win.height   = screen.h;
                //hv_dbg("x: %d, y: %d, w: %d, h: %d\n",screen.x,screen.y,screen.w,screen.h);
	}

        
	test_info.layer_config.info.fb.size[0].width = width;
	test_info.layer_config.info.fb.size[0].height = height;
	test_info.layer_config.info.fb.size[1].width = width/2;
	test_info.layer_config.info.fb.size[1].height = height/2;
	//test_info.layer_config.info.fb.size[2].width = width / 2;
	//test_info.layer_config.info.fb.size[2].height = height / 2;
	
	// src 
	//test_info.layer_config.info.fb.crop.x     = 0;
	//test_info.layer_config.info.fb.crop.y     = 0;
	test_info.layer_config.info.fb.crop.width = (unsigned long long)width << 32;
	test_info.layer_config.info.fb.crop.height= (unsigned long long)height << 32;
	
	//test_info.layer_config.info.fb.src_win.width = width;
	//test_info.layer_config.info.fb.src_win.height = height;

	test_info.layer_config.info.fb.addr[0] = (*addr);
	test_info.layer_config.info.fb.addr[1] = (test_info.layer_config.info.fb.addr[0] + width*height);
	//test_info.layer_config.info.fb.addr[2] = (int)(test_info.layer_config.info.fb.addr[0] + width*height*5/4);

	test_info.layer_config.enable = 1;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 1;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		hv_err("disp_set_addr fail to set layer info\n");

	return 0;
}

static int disp_send_command(void* display,command cmd)
{
	display_handle *disp = (display_handle*)display;
	disp->state = cmd;

	if(cmd == FULL_SCREEN){
		test_info.full_screen = 1;
	}

	else if(cmd == FULL_CAPTURE){
		test_info.full_screen = 0;
	}
	
	return 0;

}

static  struct disp_ops ops = {
	.disp_init 	   = disp_init,
	.disp_set_addr 	   = disp_set_addr,
	.disp_quit 	   = disp_quit,
	.disp_send_command = disp_send_command,
};
static  display_handle sun9iw1p1_disp = {
	.ops = &ops,
};

int display_register(hawkview_handle* hawkview)
{
	if (hawkview == NULL){
		hv_err("hawkview handle is NULL,sunxi9iw1p1 display register fail\n");
		return -1;
	}
	hawkview->display.ops = &ops;
	hv_msg("sunxi9iw1p1 display register sucessfully!\n");
	return 0;
}

