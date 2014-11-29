CC=gcc
CFLAGS=-c -Wall -I../../common_toolx/ -fPIC -D_FUTEX_BARRIER_ -D_REEACT_DEFAULT_POLICY_ -I ~/WorkingDisk/Tools/usr/include/
LDFLAGS= -L../../common_toolx/ -L ~/WorkingDisk/Tools/usr/lib/ -fPIC -shared
LIBS= -lpthread -ldl -lhwloc
REEACTSRC=reeact.c ./utils/reeact_log.c ./utils/reeact_topology.c
FASTSYNCSRC=./fastsync/fastsync_barrier.c ./fastsync/fastsync_mutex.c
PTHHOOKSRC=./pthread_hooks/pthread_hooks.c ./pthread_hooks/pthread_create.c ./pthread_hooks/pthread_barrier.c
POLICYSRC=./policies/reeact_policy.c
SOURCES=$(REEACTSRC) $(FASTSYNCSRC) $(PTHHOOKSRC) $(POLICYSRC)
BUILD=build
OBJECTS=$(addprefix $(BUILD)/, $(SOURCES:.c=.o))
DEPDIR=.depends
DEPENDS=$(addprefix $(DEPDIR)/, $(SOURCES:.c=.d))
TARGET=libreeact.so
LIBDIR=../lib/


all: debug

debug: CFLAGS+=-D_REEACT_DEBUG_ -g
debug: $(TARGET)

release: CFLAGS+=-O3
release: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(BUILD)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(TARGET) $(DEPENDS)
	rm -r $(BUILD) $(DEPDIR)

#for header file dependencies
$(DEPDIR)/%.d: %.c
	@set -e; rm -f $@; mkdir -p $(@D); \
	$(CC) -MM $(CFLAGS) -MT "$(BUILD)/$*.o $@" $< > $@; \

include $(DEPENDS)

.PHONY: all clean debug release
