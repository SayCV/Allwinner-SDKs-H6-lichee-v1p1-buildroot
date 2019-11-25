
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk

CFLAGS  := $(CFLAGS)
CFLAGS  += -I./include -I./libion
LDFLAGS := -lpthread -lscript

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
usbcamtester_objs = disp_buffer.o \
	display.o \
	main.o \
	v4l2_string.o \
	v4l2_utils.o \
	libion/ion_alloc.o

# add your target(s) to all
.PHONY: all
all: usbcamtester

usbcamtester: $(usbcamtester_objs)
	$(LINK_MSG)
	$(LINKX)

$(usbcamtester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

