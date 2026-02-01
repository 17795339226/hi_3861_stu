#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_i2c.h"
#include "hi_errno.h"
#include "hi_uart.h"
#include "hal_bsp_structAll.h"
#include "hal_bsp_sht20.h"
#include "hal_bsp_aw2013.h"
#include "hal_bsp_ap3216c.h"
#include "hal_bsp_ssd1306.h"
#include "hal_bsp_pcf8574.h"
#include "hal_bsp_nfc.h"
#include "hal_bsp_wifi.h"
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "lwip/api_shell.h"
#include "cJSON.h"
#include "sockets.h"

/*登录数据*/
typedef struct {
    char username[20];
    char password[7];
    char flags; // 成功 1 失败 0
} user_t;

/*查看环境数据*/
typedef struct {
    float temp;
    unsigned char hume;
    unsigned short lux;
    unsigned char devstatus; // 0-7bit 0则关 1-2温控指令(00==off 01==1档 10==2档 11==3档) 3加湿
} env_t;

/*设置阈值数据*/
typedef struct {
    float tempup;
    float tempdown;
    unsigned char humeup;
    unsigned char humedown;
    unsigned short luxup;
    unsigned short luxdown;
} limitset_t;

/*整体消息结构*/
typedef struct {
    long long msgtype;
    unsigned long rettype;
    unsigned char commd;
    user_t user;
    env_t envdata;
    limitset_t limitset;
    /*控制设备数据*/
    char devctrl;
    
} msg_t;

/*网络数据交互结构*/
msg_t msg;

char ssid[] = "hello"; // 自己的wifi名字 用自己的手机热点
char psk[] = "L12345678"; // wifi密码

/*服务器IP地址和端口号*/
char ip[] = "192.168.198.251";
char port[] = "1414";

/*登录用户名和密码*/
char id[] = "admin";
char ps[] = "123456";

struct sockaddr_in serveraddr;

void mynet(void) {
    /*链接wifi*/
    if(WiFi_connectHotspots(ssid, psk)) {
        printf("wifi link error\r\n");
        return;
    }

    /*申请socket*/
    int fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        printf("socket error\r\n");
        return;
    }

    /*填充信息结构*/
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(port));
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    int len = sizeof(serveraddr);

    /*链接服务器*/
    if(lwip_connect(fd, (const struct sockaddr *)&serveraddr, len)) {
        printf("链接服务器失败\r\n");
        lwip_close(fd);
        return;
    }

    /*登录系统*/
    memset(&msg, 0, sizeof(msg_t));
    strcpy(msg.user.username, id);
    strcpy(msg.user.password, ps);
    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
        printf("设备登录系统失败\r\n");
        lwip_close(fd);
        return;
    }

    memset(&msg, 0, sizeof(msg_t));
    if(0 > recv(fd, &msg, sizeof(msg_t), 0)) {
        printf("登录失败\r\n");
        lwip_close(fd);
        return;
    }

    if(msg.user.flags) {
        /*等待指令*/
        while(1) {
            memset(&msg, 0, sizeof(msg_t));
            if(0 > recv(fd, &msg, sizeof(msg_t), 0)) {
                printf("接收服务器数据失败\r\n");
                lwip_close(fd);
                return;
            }

            switch(msg.commd) {
                case 1:
                    printf("用户查看环境指令到来\r\n");
                    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
                        printf("设备登录系统失败\r\n");
                        lwip_close(fd);
                        return;
                    }
                    break;
                case 2:
                    printf("用户设置阈值指令到来\r\n");
                    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
                        printf("设备登录系统失败\r\n");
                        lwip_close(fd);
                        return;
                    }
                    break;
                case 3:
                    printf("用户控制设备指令到来\r\n");
                    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
                        printf("设备登录系统失败\r\n");
                        lwip_close(fd);
                        return;
                    }
                    break;
                case 255:
                    printf("服务器心跳检测数据到来\r\n");
                    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
                        printf("设备登录系统失败\r\n");
                        lwip_close(fd);
                        return;
                    }
                    break;
                default:
                    printf("不明指令，注意攻击\r\n");
                    if(0 > send(fd, &msg, sizeof(msg_t), 0)) {
                        printf("设备登录系统失败\r\n");
                        lwip_close(fd);
                        return;
                    }
                    break;
            }
        }
    } else {
        printf("设备ID或key错误\r\n");
        return;
    }
}
SYS_RUN(mynet);