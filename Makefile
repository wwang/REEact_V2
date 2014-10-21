COPY=cp
MAKE=make
LIB=libreeact.so
LIBDIR=./lib/
TARGET=$(LIBDIR)/$(LIB)
SRCDIR=./src

all: build install

build:
	$(MAKE) -C $(SRCDIR)

install: $(TARGET)

$(TARGET): $(SRCDIR)/$(LIB)
	mkdir -p $(LIBDIR)
	$(COPY) $(SRCDIR)/$(LIB) $(TARGET)

clean: 
	$(MAKE) -C $(SRCDIR) clean
	rm $(TARGET)

.PHONY: all build install clean