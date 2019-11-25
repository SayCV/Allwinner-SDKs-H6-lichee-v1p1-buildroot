
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g

LDFLAGS := -lscript -lsalsa -lpthread -ldisp

ifeq ($(LICHEE_CHIP),sun50iw1p1)
LDFLAGS += -ltinyalsa
endif

ifeq ($(LICHEE_CHIP),sun50iw2p1)
LDFLAGS += -ltinyalsa
endif

ifeq ($(LICHEE_CHIP),sun50iw6p1)
LDFLAGS += -ltinyalsa
endif

# include rule.mk
include $(SRC_ROOT)/rule.mk

CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src

ifeq ($(LICHEE_CHIP),sun50iw1p1)
	CFLAGS += -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW1P1
endif

ifeq ($(LICHEE_CHIP),sun50iw2p1)
	CFLAGS += -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW2P1
endif

ifeq ($(LICHEE_CHIP),sun50iw6p1)
	CFLAGS += -I$(LIBDIR)/tinyalsa-master-1.0/include -D_SUN50IW6P1
endif

# define objects collection variable
tvtester_objs = tvtester.o

# add your target(s) to all
.PHONY: all
all: tvtester
	cp tvtester.sh  $(BINDIR)/
	cp config_ac100.sh  $(BINDIR)/
	cp config_ac200.sh  $(BINDIR)/
# define you target, the target will be output to dragonboard/output/bin
# directory
tvtester: $(tvtester_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'example_objs' to your objects collection variable
$(tvtester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

