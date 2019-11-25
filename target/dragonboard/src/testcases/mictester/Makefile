
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
LDFLAGS := -lscript -lpthread -lsalsa

# include rule.mk
include $(SRC_ROOT)/rule.mk

CFLAGS := $(CFLAGS) -I$(LIBDIR)/salsa-lib-0.0.22/src

# define objects collection variable
ifeq ($(LICHEE_CHIP),sun8iw11p1)
mictester_objs = mixer.o pcm.o tiny_cap_play_sun8iw11p.o
endif

ifeq ($(LICHEE_CHIP),sun8iw6p1)
mictester_objs = mixer.o pcm.o tiny_cap_play_sun8iw6p.o
endif

ifeq ($(LICHEE_CHIP),sun50iw2p1)
mictester_objs = mixer.o pcm.o tiny_cap_play_sun50iw2p.o
endif

ifeq ($(LICHEE_CHIP),sun50iw6p1)
mictester_objs = mixer.o pcm.o tiny_cap_play_sun50iw6p.o
endif
# add your target(s) to all
.PHONY: all
all: mictester

# define you target, the target will be output to dragonboard/output/bin
# directory
mictester: $(mictester_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'example_objs' to your objects collection variable
$(mictester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

