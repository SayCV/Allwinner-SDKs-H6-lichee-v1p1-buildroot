
#include "dt.h"


#define FB_WIDTH 800
#define FB_HEIGHT 480
#define DOUBLE_BUFFER 1 //0/1
#define SCREEN_A 0

#define DISP_OUT_LCD

int main(void)
{
    int dispfh, fbfh0;
    __disp_fb_create_para_t fb_para;
    unsigned long arg[4];
    int memfh;
    unsigned long layer_hdl;
    void *mem_addr0;
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    __disp_color_t bk_color;
    int i = 0;
    int ret = 0;
    __disp_rect_t scn_win;

    unsigned int scaler_hdl;
    __disp_capture_screen_para_t capture_para;
    __u32 user_addr,phy_addr;
    __disp_rectsz_t screen_size;
    __disp_scaler_para_t scaler_para;
    __disp_layer_info_t layer_info;

    if((dispfh = open("/dev/disp",O_RDWR)) == -1)
    {
    	printf("open file /dev/disp fail. \n");
    	return 0;
    }

    if((memfh= open("pic/memin_cb.bin",O_RDONLY)) == -1)
    {
        printf("open picture file fail. \n");
        return 0;
    }

    arg[0] = SCREEN_A;
    ioctl(dispfh,DISP_CMD_LCD_ON,(unsigned long)arg);

    screen_size.width = FB_WIDTH;
    screen_size.height = FB_HEIGHT;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)&screen_size;
    ioctl(dispfh,DISP_CMD_SET_SCREEN_SIZE,(unsigned long)arg);

    bk_color.red = 0xff;
    bk_color.green = 0x00;
    bk_color.blue = 0x00;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)&bk_color;
    ioctl(dispfh,DISP_CMD_SET_BKCOLOR,(unsigned long)arg);

//request fb0
    fb_para.mode = DISP_LAYER_WORK_MODE_NORMAL;
    fb_para.smem_len = FB_WIDTH * FB_HEIGHT * 4/*32bpp*/ * (DOUBLE_BUFFER+1);
    fb_para.ch1_offset = 0;
    fb_para.ch2_offset = 0;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)&fb_para;
    layer_hdl = ioctl(dispfh,DISP_CMD_FB_REQUEST,(unsigned long)arg);
    if(layer_hdl <= 0)
    {
        printf("request fb fail\n");
    }
    
    if((fbfh0 = open("/dev/fb0",O_RDWR)) > 0)
    {
        printf("open fb0 ok\n");
    }
    else
    {
        printf("open fb0 fail!!!\n"); 
        
    }
    ioctl(fbfh0,FBIOGET_FSCREENINFO,&fix);
    mem_addr0 = mmap(NULL, fix.smem_len,PROT_READ | PROT_WRITE, MAP_SHARED, fbfh0, 0);
    memset(mem_addr0,0xff,fix.smem_len/(DOUBLE_BUFFER+1));
    read(memfh,mem_addr0,fix.smem_len/(DOUBLE_BUFFER+1),0);
    ioctl(fbfh0,FBIOGET_VSCREENINFO,&var);
    var.xoffset= 0;
    var.yoffset= 0;
    var.xres = FB_WIDTH;
    var.yres = FB_HEIGHT;
    var.xres_virtual= FB_WIDTH;
    var.yres_virtual= FB_HEIGHT * (DOUBLE_BUFFER+1);
    var.nonstd = 0;
    var.bits_per_pixel = 32;
    var.transp.length = 8;
    var.red.length = 8;
    var.green.length = 8;
    var.blue.length = 8;
    var.activate = FB_ACTIVATE_FORCE;
    ioctl(fbfh0,FBIOPUT_VSCREENINFO,&var);

    scn_win.x = 0;
    scn_win.y = 0;
    scn_win.width = FB_WIDTH;
    scn_win.height = FB_HEIGHT;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)layer_hdl;
    arg[2] = (unsigned long)&scn_win;
    ioctl(dispfh,DISP_CMD_LAYER_SET_SCN_WINDOW,(unsigned long)arg);

//----------------for capturn begen----------------
    arg[0] = 0;
    arg[1] = fix.smem_len/(DOUBLE_BUFFER+1);
    if(ioctl(dispfh,DISP_CMD_MEM_REQUEST,(unsigned long)arg) < 0)
    {
        printf("request mem fail\n");
        return 0;
    }

    arg[0] = 0;
    ioctl(dispfh,DISP_CMD_MEM_SELIDX,(unsigned long)arg);
    user_addr = (__u32)mmap(NULL, fix.smem_len/(DOUBLE_BUFFER+1), PROT_READ | PROT_WRITE, MAP_SHARED, dispfh, 0L);
    printf("scaler out user_addr:0x%x\n",user_addr);

    arg[0] = 0;
    phy_addr = ioctl(dispfh,DISP_CMD_MEM_GETADR,(unsigned long)arg);
    printf("scaler out phy_addr:0x%x\n",phy_addr);


#if 0
    capture_para.output_fb.addr[0] = phy_addr;
    capture_para.output_fb.size.width = FB_WIDTH;
    capture_para.output_fb.size.height = FB_HEIGHT;
    capture_para.output_fb.format = DISP_FORMAT_ARGB8888;
    capture_para.output_fb.seq = DISP_SEQ_ARGB;
    capture_para.output_fb.mode = DISP_MOD_INTERLEAVED;
    capture_para.output_fb.br_swap = 0;
    capture_para.output_fb.cs_mode = DISP_BT601;
    capture_para.screen_size.width = FB_WIDTH;
    capture_para.screen_size.height = FB_HEIGHT;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)&capture_para;
    ioctl(dispfh,DISP_CMD_CAPTURE_SCREEN,(unsigned long)arg);
#endif

#if 1
    printf("press any key to scaler\n");
    getchar();

    scaler_hdl = ioctl(dispfh,DISP_CMD_SCALER_REQUEST,(unsigned long)arg);
    
    scaler_para.input_fb.addr[0] = fix.smem_start;
    scaler_para.input_fb.size.width = FB_WIDTH;
    scaler_para.input_fb.size.height = FB_HEIGHT;
    scaler_para.input_fb.format = DISP_FORMAT_ARGB8888;
    scaler_para.input_fb.seq = DISP_SEQ_ARGB;
    scaler_para.input_fb.mode = DISP_MOD_INTERLEAVED;
    scaler_para.input_fb.br_swap = 0;
    scaler_para.input_fb.cs_mode = DISP_BT601;
    scaler_para.source_regn.x = 0;
    scaler_para.source_regn.y = 0;
    scaler_para.source_regn.width = FB_WIDTH;
    scaler_para.source_regn.height = FB_HEIGHT;
    scaler_para.output_fb.addr[0] = phy_addr;
    scaler_para.output_fb.size.width = FB_WIDTH;
    scaler_para.output_fb.size.height = FB_HEIGHT;
    scaler_para.output_fb.format = DISP_FORMAT_ARGB8888;
    scaler_para.output_fb.seq = DISP_SEQ_ARGB;
    scaler_para.output_fb.mode = DISP_MOD_INTERLEAVED;
    scaler_para.output_fb.br_swap = 0;
    scaler_para.output_fb.cs_mode = DISP_BT601;
    arg[1] = scaler_hdl;
    arg[2] = (unsigned long)&scaler_para;
    ioctl(dispfh,DISP_CMD_SCALER_EXECUTE,(unsigned long)arg);

    arg[1] = scaler_hdl;
    ioctl(dispfh,DISP_CMD_SCALER_RELEASE,(unsigned long)arg);
#endif
//----------------for capturn end----------------

    printf("press any key to display scaler out\n");
    getchar();

    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)layer_hdl;
    arg[2] = (unsigned long)&layer_info;
    ioctl(dispfh,DISP_CMD_LAYER_GET_PARA,(unsigned long)arg);

    layer_info.fb.addr[0] = phy_addr;
    arg[0] = SCREEN_A;
    arg[1] = (unsigned long)layer_hdl;
    arg[2] = (unsigned long)&layer_info;
    ioctl(dispfh,DISP_CMD_LAYER_SET_PARA,(unsigned long)arg);

    printf("press any key to open lcd\n");
    getchar();
    arg[0] = SCREEN_A;
    ioctl(dispfh,DISP_CMD_LCD_ON,(unsigned long)arg);

    printf("press any key to exit\n");
    getchar();

    arg[0] = 0;
    ioctl(dispfh,DISP_CMD_MEM_RELASE,(unsigned long)arg);
    
    arg[0] = SCREEN_A;
    arg[1] = layer_hdl;
    ioctl(dispfh,DISP_CMD_FB_RELEASE,(unsigned long)arg);

    arg[0] = SCREEN_A;
    ioctl(dispfh,DISP_CMD_LCD_OFF,(unsigned long)arg);
    ioctl(dispfh,DISP_CMD_HDMI_OFF,(unsigned long)arg);
    ioctl(dispfh,DISP_CMD_VGA_OFF,(unsigned long)arg);
    ioctl(dispfh,DISP_CMD_TV_OFF,(unsigned long)arg);

    close(memfh);
    close(dispfh);
    close(fbfh0);
    
    return 0;
}
