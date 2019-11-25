/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <poll.h>
#include <stdio.h>
#include <pthread.h>
#include "dragonboard_inc.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define WIFI_POWER_STATE_PATH   "/sys/devices/virtual/misc/sunxi-wlan/rf-ctrl/power_state"
#define WIFI_SCAN_DEVICE_PATH   "/sys/devices/virtual/misc/sunxi-wlan/rf-ctrl/scan_device"

struct wifi_hardware_info {
    unsigned long device_id;
    char *module_name;
    char *driver_name;
    char *vendor_name;
    char *sta_firmware;
    char *bt_firmware;
};
static const struct wifi_hardware_info wifi_list[] = {
    {
        .device_id    = 0x08179,
        .module_name  = "rtl8188eu",
        .driver_name  = "8188eu",
        .vendor_name  = "realtek",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id    = 0x18179,
        .module_name  = "rtl8189es",
        .driver_name  = "8189es",
        .vendor_name  = "realtek",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id    = 0x1f179,
        .module_name  = "rtl8189ftv",
        .driver_name  = "8189fs",
        .vendor_name  = "realtek",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id    = 0x1b723,
        .module_name  = "rtl8723bs",
        .driver_name  = "8723bs",
        .vendor_name  = "realtek",
        .sta_firmware = "",
        .bt_firmware  = "rtl8723b_fw",
    },
    {
        .device_id    = 0x1a962,
        .module_name  = "ap6210",
        .driver_name  = "bcmdhd",
        .vendor_name  = "broadcom",
        .sta_firmware = "fw_bcm40181a2.bin",
        .bt_firmware  = "bcm20710a1.hcd",
    },
    {
        .device_id    = 0x14330,
        .module_name  = "ap6330",
        .driver_name  = "bcmdhd",
        .vendor_name  = "broadcom",
        .sta_firmware = "fw_bcm40183b2_ag.bin",
        .bt_firmware  = "bcm40183b2.hcd",
    },
    {
        .device_id    = 0x14335,
        .module_name  = "ap6335",
        .driver_name  = "bcmdhd",
        .vendor_name  = "broadcom",
        .sta_firmware = "fw_bcm4339a0_ag.bin",
        .bt_firmware  = "bcm4339a0.hcd",
    },
    {
        .device_id    = 0x1a9a6,
        .module_name  = "ap6212",
        .driver_name  = "bcmdhd",
        .vendor_name  = "broadcom",
        .sta_firmware = "fw_bcm43438a0.bin",
        .bt_firmware  = "bcm43438a0.hcd",
    },
    {
        .device_id    = 0x14356,
        .module_name  = "ap6356s",
        .driver_name  = "bcmdhd",
        .vendor_name  = "broadcom",
        .sta_firmware = "fw_bcm4356a2_ag.bin",
        .bt_firmware  = "bcm4356a2.hcd",
    },
    {
        .device_id    = 0x13030,
        .module_name  = "ssv6051",
        .driver_name  = "ssv6051",
        .vendor_name  = "southsv",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id   = 0x11111,
        .module_name = "esp8089",
        .driver_name = "esp8089",
        .vendor_name = "eagle",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id   = 0x12281,
        .module_name = "xr819",
        .driver_name = "xradio_wlan",
        .vendor_name = "xradio",
        .sta_firmware = "",
        .bt_firmware  = "",
    },
    {
        .device_id   = 0x1a9bf,
        .module_name = "ap6255",
        .driver_name = "bcmdhd",
        .vendor_name = "broadcom",
        .sta_firmware = "fw_bcm43455c0_ag.bin",
        .bt_firmware = "bcm4345c0.hcd",
    },

};
/* default select rtl8188eu if get wifi_hardware_info failed */
static struct wifi_hardware_info selected_wifi = {
    0x08179, "rtl8188eu", "8188eu", "realtek", "", "",
};

static enum{running, exiting, exited} thread_state = exited;

static int get_hardware_info_by_device_id(const unsigned long device_id)
{
    unsigned int i = 0;
    if(selected_wifi.device_id == device_id) {
        return 0;
    }
    for(i = 0; i < ARRAY_SIZE(wifi_list); i++) {
        if(wifi_list[i].device_id == device_id) {
            selected_wifi = wifi_list[i];
            return 0;
        }
    }
    return -1;
}

static int wifi_scan_device(int val)
{
    int fd = 0;
    int size = 0;
    char to_write = val ? '1' : '0';

    fd = open(WIFI_SCAN_DEVICE_PATH, O_WRONLY);
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

static int wifi_power_on(void)
{
    int fd = 0;
    int size = 0;
    char to_write = '1';

    fd = open(WIFI_POWER_STATE_PATH, O_WRONLY);
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

static int wifi_power_off(void)
{
    int fd = 0;
    int size = 0;
    char to_write = '0';

    fd = open(WIFI_POWER_STATE_PATH, O_WRONLY);
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

static void parse_uevent(char *msg)
{
    char sdio_device_id[10] = {0};
    char device_type[10] = {0};
    char *subsystem = NULL;
    char *sdio_id = NULL;
    char *usb_product = NULL;

    while(*msg) {
        if(!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            subsystem = msg;
        } else if(!strncmp(msg, "SDIO_ID=", 8)) {
            msg += 8;
            sdio_id = msg;
        } else if(!strncmp(msg, "PRODUCT=", 8)) {
            msg += 8;
            usb_product = msg;
        }

        /* advance to after the next \0 */
        while(*msg++) {
            /* do nothing */
        }
    }

    if(!strncmp(subsystem, "sdio", 4)) {
        strcpy(device_type, "sdio");
        char *subid = strrchr(sdio_id, ':');
        if(subid == NULL) {
            return;
        }
        subid++;
        strcpy(sdio_device_id, subid);

        unsigned long val = strtoul(sdio_device_id, NULL, 16);
        val += 0x10000;
        if(!get_hardware_info_by_device_id(val)) {
            thread_state = exiting;
        }
    } else if(!strncmp(subsystem, "usb", 3)) {
        strcpy(device_type, "usb");
        char *result = NULL;
        unsigned long usb_pid = 0;

        strtok(usb_product, "/");
        result = strtok( NULL, "/");
        if(result) {
            usb_pid = strtoul(result, NULL, 16);
        }

        if(!get_hardware_info_by_device_id(usb_pid)) {
            thread_state = exiting;
        }
    }
}

#define UEVENT_MSG_LEN  1024
static void *ls_device_thread()
{
    char buf[UEVENT_MSG_LEN + 2] = {0};
    int count;
    int err;
    int retval;
    struct sockaddr_nl snl;
    int sock;
    struct pollfd fds;
    const int buffersize = 32*1024;

    thread_state = running;
    memset(&snl, 0x0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = 0;
    snl.nl_groups = 0xffffffff;
    sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sock < 0) {
        pthread_exit((void *)-1);
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
    retval = bind(sock, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if(retval < 0) {
        close(sock);
        pthread_exit((void *)-1);
    }

    while (running == thread_state) {
        fds.fd = sock;
        fds.events = POLLIN;
        fds.revents = 0;
        err = poll(&fds, 1, 1000);
        memset(buf, '\0', sizeof(char) * 1024);
        if(err > 0 && (fds.revents & POLLIN)) {
            count = recv(sock, buf, sizeof(char) * 1024,0);
            if(count > 0) {
                parse_uevent(buf);
            }
        }
    }

    close(sock);
    thread_state = exited;
    pthread_exit((void *)0);
}

const char *get_wifi_vendor_name()
{
    return selected_wifi.vendor_name;
}

const char *get_wifi_module_name()
{
    return selected_wifi.module_name;
}

const char *get_wifi_driver_name()
{
    return selected_wifi.driver_name;
}

const char *get_sta_firmware()
{
    return selected_wifi.sta_firmware;
}

const char *get_bt_firmware()
{
    return selected_wifi.bt_firmware;
}
void get_driver_module_arg(char* arg)
{
    char module_arg[512] = {0};
    const char *vendor_name = get_wifi_vendor_name();
    const char *bt_firmware = get_bt_firmware();

    if(strcmp(vendor_name, "realtek") == 0) {
        const char *driver_module_arg = "module_arg0=ifname=wlan0";
        snprintf(module_arg, sizeof(module_arg), "module_arg_num=0\n%s", driver_module_arg);
    } else if(strcmp(vendor_name, "broadcom") == 0) {
        const char *nvram_path = "module_arg0=nvram_path=/etc/firmware/nvram";
        const char *firmware_path = "module_arg1=firmware_path=/etc/firmware/";
        snprintf(module_arg, sizeof(module_arg), "module_arg_num=1\n%s_%s.txt\n%s%s",
                nvram_path, get_wifi_module_name(), firmware_path, get_sta_firmware());
    } else if(strcmp(vendor_name, "southsv") == 0) {
        const char *driver_module_arg = "module_arg0=stacfgpath=/etc/firmware/ssv6051-wifi.cfg";
        snprintf(module_arg, sizeof(module_arg), "module_arg_num=0\n%s", driver_module_arg);
    }

    if(strlen(bt_firmware) > 0) {
        snprintf(arg, sizeof(module_arg), "%s\nbt_firmware=%s", module_arg, bt_firmware);
    } else {
        snprintf(arg, sizeof(module_arg), "%s", module_arg);
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;
    pthread_t ls_device_thread_fd;
    int store_power_state = 0;
    char module_arg[512] = {0};

    ret = pthread_create(&ls_device_thread_fd, NULL, ls_device_thread, NULL);
    if(ret) {
        return -1;
    }

    store_power_state = wifi_power_on();
    if(store_power_state < 0) {
        return -1;
    } else {
        usleep(100000);
        wifi_scan_device(1);
    }

    int i = 0;
    for(i = 0; i < 50; i++) {
        if(exited == thread_state) {
            break;
        } else {
           usleep(100000);
        }
    }

    if(running == thread_state) {
        thread_state = exiting;
    }

    pthread_join(ls_device_thread_fd, NULL);
    wifi_power_off();
    wifi_scan_device(0);

    const char *vendor_name = get_wifi_vendor_name();
    const char *module_name = get_wifi_module_name();
    const char *driver_name = get_wifi_driver_name();
    get_driver_module_arg(module_arg);
    db_debug("module_name: %s\n", module_name);
    db_debug("%s\n", module_arg);

    FILE *file = fopen("/data/wifi_hardware_info", "w");
    if(file == NULL){
        return -1;
    }
    fprintf(file,"vendor_name=%s\nmodule_name=%s\ndriver_name=%s\n%s\n",
            vendor_name, module_name, driver_name, module_arg);
    fflush(file);
    fsync(fileno(file));
    fclose(file);
    return 0;
}
