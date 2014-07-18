CC=arm-none-linux-gnueabi-gcc

TARGET          = hawkview

INCLUDES        = -I.
SUBLIBS         = -lpthread -lm -lrt
CFLAGS          = -Wall -D_REENTRANT 
LDFLAGS         =

PLATFORM = $(shell echo ${HAWKVIEW_PLATFORM})
PLATFORM_DIR = platform/$(PLATFORM)

PLATFORM_DISP = $(PLATFORM_DIR)/$(PLATFORM)_disp.c
#PLATFORM_VIDEO = $(PLATFORM_DIR)/$(PLATFORM)_video.c

#LATFORM_DISP = sun9iw1p1_disp.c
PLATFORM_VIDEO = sun9iw1p1_video.c
SRCS    = main.c hawkview.c video_helper.c $(PLATFORM_VIDEO) $(PLATFORM_DISP)
OBJS    = $(SRCS:.c=.o)

.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

$(TARGET):      $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(SUBLIBS) -static

all:
	$(TARGET)

clean:
	rm -rf $(TARGET) *.o *.a *~
	cd $(PLATFORM_DIR) && rm -rf *.o *.a *~

distclean:
	rm -f $(TARGET) *.o *.a *.bak *~ .depend
	cd $(PLATFORM_DIR) && rm -f *.o *.a *.bak *~ .depend
	
dep:
	depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend
show:
	echo $(SUBLIBS)
	echo $(PLATFORM_DIR)