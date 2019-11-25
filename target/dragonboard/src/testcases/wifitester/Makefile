
# define sources root directory before everything
SRC_ROOT := ../..

LDFLAGS := -lpthread

# include rule.mk
include $(SRC_ROOT)/rule.mk
ifeq ($(LICHEE_CHIP),sun50iw1p1)
	CFLAGS := -D_SUN50IW1P1
endif

# define objects collection variable
wifitester_objs = wifitester.o

# add you test case here
.PHONY: all
all: wifitester
	cp wifitester.sh $(BINDIR)/	
	cp -f ap6181/* $(BINDIR)/
	cp -f ap6210/* $(BINDIR)/
	cp -f ap6330/* $(BINDIR)/
	cp -f ap6335/* $(BINDIR)/
	cp -f ap6212/* $(BINDIR)/
	cp -f ap6356s/* $(BINDIR)/
	cp -f ssv6051/* $(BINDIR)/
	cp -f esp8089/* $(BINDIR)/
	cp -f xr819/* $(BINDIR)/
	cp -f ap6255/* $(BINDIR)

wifitester: $(wifitester_objs)
		$(LINK_MSG)
		$(LINKX)

# change 'example_objs' to your objects collection variable
$(wifitester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)
