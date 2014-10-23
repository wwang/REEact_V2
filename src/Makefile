CC=gcc
CFLAGS=-c -Wall -D_REEACT_DEBUG_ -O3 -I../../common_toolx/ -fPIC -D_FUTEX_BARRIER_ -D_FLEX_PTHREAD_POLICY_
LDFLAGS= -L../../common_toolx/ -fPIC -shared
LIBS= -lpthread -ldl
REEACTSRC=reeact.c ./utils/reeact_log.c
FASTSYNCSRC=./fastsync/fastsync_barrier.c 
PTHHOOKSRC=./pthread_hooks/pthread_hooks.c ./pthread_hooks/pthread_create.c ./pthread_hooks/pthread_barrier.c
POLICYSRC=./policies/reeact_policy.c ./policies/flex_pthread/flexpth_barrier.c ./policies/flex_pthread/flexpth_create.c ./policies/flex_pthread/flex_pthread.c
SOURCES=$(REEACTSRC) $(FASTSYNCSRC) $(PTHHOOKSRC) $(POLICYSRC)
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
