#include "hawkview.h"
extern int hawkview_init(hawkview_handle* haw);
int main()
{
	hawkview_handle hawkview;
	hawkview.capture.cap_w = 640;
	hawkview.capture.cap_h = 480;
	hawkview.capture.video_no = 1;
	hawkview.capture.subdev_id = 0;
	hawkview.capture.cap_fps = 30;
	hawkview.capture.cap_fmt = V4L2_PIX_FMT_NV12;
	

	hawkview.display.input_w = 640;
	hawkview.display.input_h = 480;
	hawkview_init(&hawkview);
	return 0;
}
