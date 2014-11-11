COPY=cp
MAKE=make
LIB=libreeact.so
LIBDIR=./lib/
TARGET=$(LIBDIR)/$(LIB)
SRCDIR=./src

all: debug install

debug release:
	$(MAKE) -C $(SRCDIR) $@

install: $(TARGET)

$(TARGET): $(SRCDIR)/$(LIB)
	mkdir -p $(LIBDIR)
	$(COPY) $(SRCDIR)/$(LIB) $@

clean: 
	$(MAKE) -C $(SRCDIR) clean
	rm $(TARGET)

.PHONY: all build install clean