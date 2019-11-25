#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <string.h>

#define BITFIELD_SET(termios, flags)   ((termios) |= (flags))
#define BITFIELD_CLR(termios, flags)   ((termios) &= ~(flags))
#define BITFIELD_ISSET(termios, flags) ((flags) == ((termios) & (flags)))
#define BITFIELD_ISCLR(termios, flags) (0 == ((termios) & (flags)))

#define GPS_NSTANDBY "/sys/devices/virtual/misc/sunxi-gps/rf-ctrl/nstandby_state"

//BCM4751
//unsigned char Training_Data[16]={0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,};
//unsigned char  SmallCmd[]={0xff,0x00,0xfd,0xc0,0x00,0xfc};
//unsigned char Relply_Barracuda[]={0xfe,0x00,0xfd,0xc0,0x00,0xf1,0x87,0x0d,0x20,0xfc};

//BCM4752
unsigned char Autobaud[16]={0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,};
unsigned char  Flow_Control[10]={0xfe,0x00,0xfd,0x21,0x00,0x01,0x33,0x0e,0x10,0xfc};
unsigned char  Write_Reg[10]={0xfe,0x00,0xfd,0x00,0x4c,0x81,0x81,0x80,0x80,0xfc};
unsigned char  Version_Req[6]={0xff,0x00,0xfd,0x00,0x00,0xfc};
unsigned char Flow_Control_Echo[10]={0xfe,0x00,0xfd,0x21,0x00,0x01,0x33,0x0e,0x10,0xfc};
unsigned char  Version_Res[10]={0xfe,0x00,0xfd,0x00,0x00,0x05,0x00,0x02,0x30,0xfc};

struct speed {
    char speed_str[20];
    speed_t speed_val;
};
static const struct speed speeds[] =
{
    {"115200", B115200},
    {"57600", B57600},
    {"38400", B38400},
    {"19200", B19200},
    {"9600", B9600},
    {"4800", B4800},
    {"2400", B2400},
    {"1200", B1200},
    {"300", B300}
};

static speed_t get_speed(char *str)
{
    unsigned int i;

    for (i= 0; i < sizeof(speeds)/sizeof(struct speed); i++) {
        if (strcmp(speeds[i].speed_str, str) == 0) {
            return speeds[i].speed_val;
        }
    }
    return B115200;
}

static int setup_uart(int fd, speed_t speed)
{
    struct termios termios;

    cfmakeraw(&termios);
    cfsetispeed(&termios, speed);
    cfsetospeed(&termios, speed);
    termios.c_cc[VMIN] = 0;
    termios.c_cc[VTIME] = 50;
    BITFIELD_SET(termios.c_iflag, IGNBRK | IXANY);
    BITFIELD_CLR(termios.c_iflag, ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXOFF | IXON);
    BITFIELD_SET(termios.c_cflag, CLOCAL | CREAD |CRTSCTS );
    BITFIELD_CLR(termios.c_lflag, ICANON);
    BITFIELD_CLR(termios.c_oflag, OCRNL | OFDEL | OFILL | OLCUC | ONLCR);
    if (tcsetattr(fd, TCSANOW, &termios) == -1) {
        return -1;
    }
    if (tcgetattr(fd, &termios) == -1) {
        return -1;
    }
    return 0;
}

static int set_nstandby(int value)
{
    int fd = 0;
    int size = 0;
    char to_write = value > 0 ? '1' : '0';

    fd = open(GPS_NSTANDBY, O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    size = write(fd, &to_write, sizeof(to_write));
    if (size < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    unsigned char buf[50];
    int count;
    speed_t speed;

    if (argc != 3) {
        return -1;
    }
    if (set_nstandby(1)) {
        return -1;
    }
    sleep(1);
    if ((fd = open(argv[1], O_RDWR | O_NOCTTY)) <= 0) {
        set_nstandby(0);
        return -1;
    }
    speed = get_speed(argv[2]);
    if (setup_uart(fd, speed)) {
        goto failed;
    }

    count=write(fd, Autobaud, 16);
    if (count != 16) {
        printf("Can not send out autobaud\n");
	goto failed;
    }

    count=write(fd, Flow_Control, 10);
    if (count != 10) {
        printf("Can not send out flow control command\n");
        goto failed;
    }
    /* Receiving Flow Control Echo */
    count=read(fd, buf, 10);
    if (count <= 0) {
        printf("can not get Flow Control Echo\n");
        goto failed;
    }

    /* Sending Write Request command */
    count=write(fd, Write_Reg, 10);
    if (count != 10) {
        printf("Can not send out write request command\n");
        goto failed;
    }

    /* Sending Version Request command */
    count=write(fd, Version_Req, 10);
    if(count != 10) {
        printf("Can not send out version request command\n");
        goto failed;
    }
    /* Receiving Version Response */
    count=read(fd, buf, 10);
    if (count <= 0) {
        printf("can not get Version Response\n");
        goto failed;
    }
    set_nstandby(0);
    return 0;

failed:
    set_nstandby(0);
    close(fd);
    return -1;
}