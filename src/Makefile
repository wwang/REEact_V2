CC=gcc
CFLAGS=-c -Wall -D_REEACT_DEBUG_ -O3 -I../../common_toolx/ -fPIC -DFUTEX_BARRIER
LDFLAGS= -L../../common_toolx/ -fPIC -shared
LIBS= -lpthread -ldl
REEACTSRC=reeact.c
FASTSYNCSRC=./fastsync/fastsync_barrier.c 
PTHHOOKSRC=./pthread_hooks/pthread_hooks.c ./pthread_hooks/pthread_create.c ./pthread_hooks/pthread_barrier.c
SOURCES=$(REEACTSRC) $(FASTSYNCSRC) $(PTHHOOKSRC)
OBJECTS=$(SOURCES:.c=.o)
TARGET=libreeact.so
LIBDIR=../lib/


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(TARGET) 

# for header file dependencies
depend: .depend	

.depend: $(SOURCES)	
	rm -f ./.depend	
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

.PHONY: all clean depend
