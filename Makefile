OBJS=misc_log.o misc_timer2.o misc_oil.o misc_net.o misc_util.o

CFLAGS += $(CFLAGHDRINC) -fPIC -g

all: libmisc.so

libmisc.so : $(OBJS)
	$(CC) -Os -s -shared -Wl,-soname,$@ -o $@ $^

install:
	install -D libmisc.so $(INSTALLDIR)/lib/
	$(STRIP) $(INSTALLDIR)/lib/libmisc.so

clean:
	rm -rf *~ *.d *.so $(OBJS)

-include $(BUILDPATH)/make.deprules

-include $(OBJS:.o=.d)
