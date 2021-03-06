CC=arm-none-linux-gnueabi-gcc

TARGET          = hawkview

INCLUDES        = -I. -Icommon/
SUBLIBS         = -lpthread -lm -lrt
CFLAGS          = -Wall -D_REENTRANT 
LDFLAGS         =

PLATFORM = $(shell echo ${HAWKVIEW_PLATFORM})
PLATFORM_DIR = platform/$(PLATFORM)

PLATFORM_DISP = $(PLATFORM_DIR)/$(PLATFORM)_disp.c
#PLATFORM_VIDEO = $(PLATFORM_DIR)/$(PLATFORM)_video.c

PLATFORM_VIDEO = common/video.c
SRCS    = main.c common/hawkview.c common/video_helper.c $(PLATFORM_VIDEO) $(PLATFORM_DISP)
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
	cd common && rm -f *.o *.a *.bak *~ .depend
	
distclean:
	rm -f $(TARGET) *.o *.a *.bak *~ .depend
	cd $(PLATFORM_DIR) && rm -f *.o *.a *.bak *~ .depend
	cd common && rm -f *.o *.a *.bak *~ .depend
	
dep:
	depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend
show:
	echo $(SUBLIBS)
	echo $(PLATFORM_DIR)