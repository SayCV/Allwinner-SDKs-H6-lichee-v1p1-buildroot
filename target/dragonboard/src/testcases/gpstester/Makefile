
# define sources root directory before everything
SRC_ROOT := ../..

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
gpstester_objs = gpstester.o

# add you test case here
.PHONY: all
all: gpstester
	cp gpstester.sh $(BINDIR)/

gpstester: $(gpstester_objs)
		$(LINK_MSG)
		$(LINKX)

# change 'example_objs' to your objects collection variable
$(gpstester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)