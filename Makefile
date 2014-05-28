CC=arm-none-linux-gnueabi-gcc

TARGET          = hawkview

INCLUDES        = -I.
SUBLIBS         = -lpthread -lm -lrt
CFLAGS          = -Wall -D_REENTRANT 
LDFLAGS         =

SRCS    = main.c hawkview.c sun9iw1p1_disp.c sun9iw1p1_video.c
OBJS    = $(SRCS:.c=.o)

.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

$(TARGET):      $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(SUBLIBS) -static

all:
	$(TARGET)

clean:
	rm -f $(TARGET) *.o *.a *~

distclean:
	rm -f $(TARGET) *.o *.a *.bak *~ .depend

dep:
	depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend
