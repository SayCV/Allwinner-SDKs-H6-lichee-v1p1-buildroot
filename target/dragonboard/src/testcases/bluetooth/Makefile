
# define sources root directory before everything
SRC_ROOT := ../..

# include rule.mk
include $(SRC_ROOT)/rule.mk

.PHONY: all
all:
	cp bttester.sh $(BINDIR)/
	cp hcitool $(BINDIR)/
	cp cciconfig $(BINDIR)/
	cp bluetooltester $(BINDIR)/

	cp -f ap6210/*.hcd $(BINDIR)/
	cp -f ap6330/*.hcd $(BINDIR)/
	cp -f ap6335/*.hcd $(BINDIR)/
	cp -f ap6212/*.hcd $(BINDIR)/
	cp -f ap6356s/*.hcd $(BINDIR)/
	cp -f ap6255/*.hcd $(BINDIR)/

	make -C rtk_hciattach
	cp -f rtk_hciattach/rtk_hciattach $(BINDIR)/
	cp -f 8723B/rtl8723b_fw $(BINDIR)/rtlbt_fw
	cp -f 8723B/rtl8723b_config $(BINDIR)/rtlbt_config
