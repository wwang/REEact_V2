CC=gcc
CFLAGS=-c -Wall -D_REEACT_DEBUG_ -O3 -I../../common_toolx/ -fPIC -D_FUTEX_BARRIER_ -D_FLEX_PTHREAD_POLICY_ -I ~/WorkingDisk/Tools/usr/include/
LDFLAGS= -L../../common_toolx/ -L ~/WorkingDisk/Tools/usr/lib/ -fPIC -shared
LIBS= -lpthread -ldl -lhwloc -lcommontoolx
REEACTSRC=reeact.c ./utils/reeact_log.c ./utils/reeact_topology.c
FASTSYNCSRC=./fastsync/fastsync_barrier.c 
PTHHOOKSRC=./pthread_hooks/pthread_hooks.c ./pthread_hooks/pthread_create.c ./pthread_hooks/pthread_barrier.c
POLICYSRC=./policies/reeact_policy.c ./policies/flex_pthread/flexpth_barrier.c ./policies/flex_pthread/flexpth_create.c ./policies/flex_pthread/flex_pthread.c ./policies/flex_pthread/flexpth_thread_keeper.c ./policies/flex_pthread/flexpth_env_var.c
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
	rm $(OBJECTS) $(TARGET)  .depend

# for header file dependencies
depend: .depend	

.depend: $(SOURCES)	
	rm -f ./.depend	
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

.PHONY: all clean depend
