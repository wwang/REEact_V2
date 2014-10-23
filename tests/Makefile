CC=gcc
CFLAGS=-c -Wall -g -D__DEBUG__ -O -I../../common_toolx/ -fPIC
LDFLAGS= -L../../common_toolx/
LIBS= -lpthread -lcommontoolx -ldl
SOURCES=sync_v2.c
OBJECTS=$(SOURCES:.c=.o)
LIBSOURCES=sync_worker_lib.c
LIBOBJECTS=$(LIBSOURCES:.c=.o)
EXECUTABLE=sync
LIB=libsyncworker.so

all: $(EXECUTABLE) $(LIB)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(LIB): $(LIBOBJECTS)
	$(CC) $(LDFLAGS) -shared $(LIBOBJECTS) -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o $(EXECUTABLE) $(LIB)

.PHONY: all clean
