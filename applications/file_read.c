/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-20     zm       the first version
 */
#include <rtthread.h>
#include <dfs_posix.h>


extern rt_mq_t mq1;

char write_stack[2048];
rt_thread_t thread;

static void net_write_entry(void *parameter)
{
    rt_kprintf("net_write_entry\r\n");
    char buf[20];
    int fd;
    fd = open("/net_read.txt", O_WRONLY | O_CREAT);
    if(fd >= 0)
    {
        rt_kprintf("create file sucees\r\n");

    }
    else {
        rt_kprintf("create file failed\r\n");
    }

    while(1)
    {
        if(rt_mq_recv(mq1, buf, sizeof(buf), RT_WAITING_FOREVER) == RT_EOK)
        {
            if(fd >= 0)
            {
                write(fd,buf,sizeof(buf));
                close(fd);
                rt_kprintf("Write done\r\n");
            }
        }
        rt_thread_mdelay(500);
    }
}

int file_read_start(void)
{
    rt_err_t ret;
    thread = rt_thread_create("thread_fileread", net_write_entry, RT_NULL, 1024, 11, 20);
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
