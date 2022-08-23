/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-13     zm       the first version
 */
#include "server.h"
#include <rtthread.h>
#include <dfs_posix.h>


rt_thread_t thread = RT_NULL;
 rt_event_t net_event;
 static int client_info;//客户端编号
 static char Recv_buf[63];//接收到的网络数据

 static char recvbuff[BUFF_SIZE];
 const static char sendbuff[] = "Hello client!";

 static void data_proc_entry(void *parameter)
 {
     rt_uint32_t recv = 0;
     int result = 0;
     static char ip[2];
     net_event = rt_event_create("net_event", RT_IPC_FLAG_FIFO);
        if(net_event != RT_NULL)
        {rt_kprintf("net事件创建成功\r\n");}
        else {
            rt_kprintf("net事件创建失败\r\n");
        }


     while(1)
     {
         rt_event_recv(net_event, RECV_EVENT, RT_EVENT_FLAG_CLEAR | RT_EVENT_FLAG_AND, RT_WAITING_FOREVER, &recv);
         if(recv == RECV_EVENT)
         {
             //rt_kprintf("test1\r\n");
             strncat(Recv_buf,"by client:",sizeof("by client:"));
             itoa(client_info,ip,10);
             strncat(Recv_buf,ip,sizeof(ip));
             //sprintf(Recv_buf,"says by:%d\r\n",client_info);
             result = rt_event_send(net_event, WRITE_EVENT);
             if(result == RT_EOK)
             {
                  rt_kprintf("send event success\r\n");
             }
         }
     }
 }

static void net_server_thread_entey(void *parameter)
{
    int sfd, cfd, maxfd, i, nready, n;
    struct sockaddr_in server_addr, client_addr;
    struct netdev *netdev = RT_NULL;

    socklen_t client_addr_len;
    fd_set all_set, read_set;
    //rt_uint32_t recv_flag = 0;

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
        rt_kprintf("enter select before\r\n");//test
        nready = select(maxfd + 1,&read_set, NULL, NULL, NULL);
        rt_kprintf("enter select after\r\n");//test
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
                client_info = clientfds[i];
                strcpy(Recv_buf,recvbuff);
                memset(recvbuff,0,sizeof(recvbuff));


                /*sprintf(Recv_buf,"clientfd %d:",clientfds[i]);
                strcat(Recv_buf,recvbuff);
                memset(recvbuff,0,sizeof(recvbuff));
                result = rt_event_send(net_event, RECV_EVENT);
                if(result == RT_EOK)
                {
                    rt_kprintf("send event success\r\n");
                  while(rt_event_recv(net_event, WRITE_EVENT, RT_EVENT_FLAG_CLEAR | RT_EVENT_FLAG_AND, RT_WAITING_FOREVER, &recv_flag) != RT_EOK);
                }
                */
                if(n <= 0)
                {
                    //从集合里面清除
                    FD_CLR(clientfds[i],&all_set);
                    //当前的客户端fd 赋值为-1
                    clientfds[i] = -1;                }
                else
                {
                    //写回客户端
                    rt_kprintf("test4\r\n");

                    n = send(clientfds[i],(char *)&sendbuff, strlen(sendbuff), 0);
                    rt_kprintf("test3]\r\n");
                    rt_event_send(net_event, RECV_EVENT);

                    //rt_thread_mdelay(200);
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

    }
}



static int server(int argc,char **argv)
{
    rt_err_t ret = RT_EOK;
    rt_kprintf("enter server\r\n");
    if(argc != 2)
    {
        rt_kprintf("bind test[netdev_name] -- bind network interface device by name\n");
        return -RT_ERROR;
    }
    else {
        //rt_kprintf("enter error pealse try again\r\n");
    }
   rt_thread_t  thread = rt_thread_create("server", net_server_thread_entey, argv[1], 4096, 10, 10);
   rt_thread_t thread2 = rt_thread_create("data_process", data_proc_entry, RT_NULL, 1024, 11, 20);

   if(thread != RT_NULL)
   {
       rt_thread_startup(thread);
   }
   else {
    ret = RT_ERROR;
   }

   if(thread2 != RT_NULL)
     {
         rt_thread_startup(thread2);
     }
     else {
      ret = RT_ERROR;
     }
   return ret;
}



static void net_write_entry(void *parameter)
{
    rt_kprintf("net_write_entry\r\n");
    //char buf[20];
    int fd;
    rt_uint32_t recv;
    off_t dis = 0;

    fd = open("/net_read.txt", O_WRONLY | O_CREAT | O_APPEND);
    if(fd >= 0)
    {
        rt_kprintf("create file sucees\r\n");

    }
    else {
        rt_kprintf("create file failed\r\n");
    }

    while(1)
    {
        rt_event_recv(net_event, WRITE_EVENT, RT_EVENT_FLAG_CLEAR | RT_EVENT_FLAG_AND, RT_WAITING_FOREVER, &recv);
        if(recv == WRITE_EVENT)
        {
            if(fd >= 0)
            {
                write(fd,Recv_buf,strlen(Recv_buf));//环形缓冲区失败，可能是内存不够？无法创建文件
                //write(fd, &client_info, 1);
                write(fd, "\r\n", strlen("\r\n"));
                dis = lseek(fd, 0, SEEK_CUR);

                fsync(fd);
                //close(fd);
                rt_kprintf("Write done\r\n");
                rt_kprintf("current write pos is:%d\r\n",dis);
            }
        }

    }
}

int file_read_start(void)
{
    rt_err_t ret;
    thread = rt_thread_create("thread_fileread", net_write_entry, RT_NULL, 4096, 12, 20);
    if(thread != RT_NULL)
    {
        rt_thread_startup(thread);
        return RT_EOK;
    }
    else {
        ret  = RT_ERROR;
    }
    return ret;
}
MSH_CMD_EXPORT(file_read_start,net file read to txt);
#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT(server,network interface device test);
#endif
