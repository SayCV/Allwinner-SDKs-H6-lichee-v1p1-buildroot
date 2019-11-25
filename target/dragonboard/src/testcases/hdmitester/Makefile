
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
ifeq ($(LICHEE_CHIP),sun50iw1p1)
LDFLAGS := -lscript -lsalsa -lpthread -ldisp -ltinyalsa
else ifeq ($(LICHEE_CHIP),sun50iw2p1)
LDFLAGS := -lscript -lsalsa -lpthread -ldisp -ltinyalsa
else ifeq ($(LICHEE_CHIP),sun50iw6p1)
LDFLAGS := -lscript -lsalsa -lpthread -ldisp -ltinyalsa
else
LDFLAGS := -lscript -lsalsa -lpthread -ldisp
endif

# include rule.mk
include $(SRC_ROOT)/rule.mk
ifeq ($(LICHEE_CHIP),sun50iw1p1)
	CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW1P1
else ifeq ($(LICHEE_CHIP),sun50iw2p1)
	CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW2P1
else ifeq ($(LICHEE_CHIP),sun50iw6p1)
	CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW6P1
else
	CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src
endif

# define objects collection variable
hdmitester_objs = hdmitester.o

# add your target(s) to all
.PHONY: all
all: hdmitester
	cp hdmitester.sh $(BINDIR)/

# define you target, the target will be output to dragonboard/output/bin
# directory
hdmitester: $(hdmitester_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'example_objs' to your objects collection variable
$(hdmitester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

