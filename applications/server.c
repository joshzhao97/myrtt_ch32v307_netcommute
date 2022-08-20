/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-13     zm       the first version
 */
#include <rtthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdev.h>
#include <stdio.h>
#include <string.h>
//#include <select.h>

#define SERVER_PORT 8888
#define BUFF_SIZE 1024

static char recvbuff[BUFF_SIZE];
#include <rtthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdev.h>
#include <stdio.h>
#include <string.h>
#define SERVER_PORT   8888
#define BUFF_SIZE 1024


 rt_uint8_t msg_pool[2048];
rt_mq_t mq1 = RT_NULL;

static char recvbuff[BUFF_SIZE];
static void net_server_thread_entey(void *parameter)
{
    int sfd, cfd, maxfd, i, nready, n,result;
    struct sockaddr_in server_addr, client_addr;
    struct netdev *netdev = RT_NULL;
    char sendbuff[] = "Hello client!";
    socklen_t client_addr_len;
    fd_set all_set, read_set;

    mq1 = rt_mq_create("write_mq", 20, 20, RT_IPC_FLAG_FIFO);
    //result = rt_mq_create("write_mq", 8, sizeof(msg_pool), flag)(&mq1, "write_mq", &msg_pool[0], 8, sizeof(msg_pool), RT_IPC_FLAG_PRIO);
    if(mq1 != RT_NULL)
    {rt_kprintf("消息队列创建成功\r\n");}
    else {
        rt_kprintf("消息队列创建失败\r\n");
    }

    rt_kprintf("init message queue success\r\n");
    //FD_SETSIZE里面包含了服务器的fd
    int clientfds[FD_SETSIZE - 1];
    /* 通过名称获取 netdev 网卡对象 */
    netdev = netdev_get_by_name((char*)parameter);
    if (netdev == RT_NULL)
    {
        rt_kprintf("get network inteRFace device(%s)faiLED.\n", (char*)parameter);
    }
    //创建socket
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        rt_kprintf("Socket create failed.\n");
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* 获取网卡对象中 IP 地址信息 */
    server_addr.sin_addr.s_addr = netdev->ip_addr.addr;
    //绑定socket
    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        rt_kprintf("socket bind failed.\n");
        closesocket(sfd);
    }
    rt_kprintf("socket bind network interfacedevice(%s) success!\n", netdev->name);
    //监听socket
    if(listen(sfd, 5) == -1)
    {
        rt_kprintf("listen error");
    }
    else
    {
        rt_kprintf("listening...\n");
    }
    client_addr_len = sizeof(client_addr);
    //初始化 maxfd 等于 sfd
    maxfd = sfd;
    //清空fdset
    FD_ZERO(&all_set);
    //把sfd文件描述符添加到集合中
    FD_SET(sfd, &all_set);
    //初始化客户端fd的集合
    for(i = 0; i < FD_SETSIZE -1 ; i++)
    {
        //初始化为-1
        clientfds[i] = -1;
    }
    while(1)
    {
        //每次select返回之后，fd_set集合就会变化，再select时，就不能使用，
        //所以我们要保存设置fd_set 和 读取的fd_set
        read_set = all_set;
        nready = select(maxfd + 1,&read_set, NULL, NULL, NULL);
        //没有超时机制，不会返回0
        if(nready < 0)
        {
            rt_kprintf("select error \r\n");
        }
        //判断监听的套接字是否有数据
        if(FD_ISSET(sfd, &read_set))
        {
            //有客户端进行连接了
            cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
            if(cfd < 0)
            {
                rt_kprintf("accept socketerror\r\n");
                //继续select
                continue;
            }
            rt_kprintf("new client connect fd =%d\r\n", cfd);
            //把新的cfd 添加到fd_set集合中
            FD_SET(cfd, &all_set);
            //更新要select的maxfd
            maxfd = (cfd > maxfd)?cfd:maxfd;
            //把新的cfd 保存到cfds集合中
            for(i = 0; i < FD_SETSIZE -1 ; i++)
            {
                if(clientfds[i] == -1)
                {
                    clientfds[i] = cfd;
                    //退出，不需要添加
                    break;
                }
            }
            //没有其他套接字需要处理：这里防止重复工作，就不去执行其他任务
            if(--nready == 0)
            {
                //继续select
                continue;
            }
        }
        //遍历所有的客户端文件描述符
        for(i = 0; i < FD_SETSIZE -1 ; i++)
        {
            if(clientfds[i] == -1)
            {
                //继续遍历
                continue;
            }
            //判断是否在fd_set集合里面
            if(FD_ISSET(clientfds[i], &read_set))
            {
                n = recv(clientfds[i],recvbuff, sizeof(recvbuff), 0);
                rt_kprintf("clientfd %d:  %s \r\n",clientfds[i], recvbuff);
                result = rt_mq_send(mq1, recvbuff, sizeof(recvbuff));
                if(result != RT_EOK)
                {
                    rt_kprintf("send msg error\r\n");
                    rt_kprintf("error code is%d",result);
                }

                if(n <= 0)
                {
                    //从集合里面清除
                    FD_CLR(clientfds[i],&all_set);
                    //当前的客户端fd 赋值为-1
                    clientfds[i] = -1;                }
                else
                {
                    //写回客户端
                    n = send(clientfds[i],sendbuff, strlen(sendbuff), 0);
                    if(n < 0)
                    {
                        //从集合里面清除
                        FD_CLR(clientfds[i],&all_set);
                        //当前的客户端fd 赋值为-1
                        clientfds[i] = -1;
                    }
                }
            }
        }
        rt_thread_mdelay(500);
    }
}

static int server(int argc,char **argv)
{
    rt_err_t ret = RT_EOK;
    if(argc != 2)
    {
        rt_kprintf("bind test[netdev_name] -- bind network interface device by name\n");
        return -RT_ERROR;
    }
   rt_thread_t  thread = rt_thread_create("server", net_server_thread_entey, argv[1], 4096, 10, 10);

   if(thread != RT_NULL)
   {
       rt_thread_startup(thread);
   }
   else {
    ret = RT_ERROR;
   }
   return ret;
}
#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(server,network interface device test);
#endif
