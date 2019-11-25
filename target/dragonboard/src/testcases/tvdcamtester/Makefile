
# define sources root directory before everything
SRC_ROOT := ../..

LDFLAGS := -lpthread

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
tvd_camera_objs = tvd_camera.o

# add you test case here
.PHONY: all
all: tvdcamtester

tvdcamtester: $(tvd_camera_objs)
		$(LINK_MSG)
		$(LINKX)

# change 'example_objs' to your objects collection variable
$(tvd_camera_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)
