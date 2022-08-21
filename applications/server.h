/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-21     zm       the first version
 */
#ifndef APPLICATIONS_SERVER_H_
#define APPLICATIONS_SERVER_H_

#include <rtthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdev.h>
#include <stdio.h>
#include <string.h>
//#include <select.h>

#define SERVER_PORT 8888
#define BUFF_SIZE 64


#include <rtthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdev.h>
#include <stdio.h>
#include <string.h>


#define RECV_EVENT (0x01 << 0)//事件掩码位0
 extern rt_event_t net_event;
 extern char Recv_buf[64];//接收到的网络数据



#endif /* APPLICATIONS_SERVER_H_ */