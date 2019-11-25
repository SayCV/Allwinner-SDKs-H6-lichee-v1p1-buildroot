

rtk_hciattach:hciattach.c hciattach_rtk.o  
	arm-linux-gnueabi-gcc -o rtk_hciattach hciattach.c hciattach_rtk.o  

hciattach_rtk.o:hciattach_rtk.c
	arm-linux-gnueabi-gcc -c hciattach_rtk.c

clean:
	rm -f *.o  rtk_hciattach
