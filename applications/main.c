/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/
/*
 1.目前使用了：网口服务器  使用server e0开启网络
 2.使用spi1挂载sd，fats文件系统  使用sd_start开启sd
 3.使用file_read_start 开启消息队列写入txt
*/
#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"
#include <board.h>
#include <rtdbg.h>
#include <rtdevice.h>
#include <drv_spi.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include <dfs_posix.h>
#include "drv_spi.h"
#include "spi_msd.h"
#include <rtdbg.h>


/* Global typedef */

/* Global define */


#define LED0_PIN  35   //PC3

/* Global Variable */

/*usr functions*/



static void sd_mount(void *parameter)
{
    while (1)
    {
        rt_thread_mdelay(500);
        if(rt_device_find("sd0") != RT_NULL)
        {
            if (dfs_mount("sd0", "/", "elm", 0, 0) == RT_EOK)
            {
                rt_kprintf("sd card mount success\r\n");
                LOG_I("sd card mount to '/'");
                break;
            }
            else
            {
                LOG_W("sd card mount to '/' failed!");
            }
        }
    }
}


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{

    rt_kprintf("MCU: CH32V307\n");
	rt_kprintf("SysClk: %dHz\n",SystemCoreClock);
    rt_kprintf("www.wch.cn\n");
	LED1_BLINK_INIT();

	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
	while(1)
	{
	    GPIO_SetBits(GPIOA,GPIO_Pin_0);
	    rt_thread_mdelay(500);
	    GPIO_ResetBits(GPIOA,GPIO_Pin_0);
	    rt_thread_mdelay(500);
	}
}


/*********************************************************************
 * @fn      led
 *
 * @brief   gpio operation by pins driver.
 *
 * @return  none
 */
int led(void)
{
    rt_uint8_t count;

    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    rt_kprintf("led_SP:%08x\r\n",__get_SP());
    for(count = 0 ; count < 10 ;count++)
    {
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_kprintf("led on, count : %d\r\n", count);
        rt_thread_mdelay(500);

        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_kprintf("led off\r\n");
        rt_thread_mdelay(500);
    }
    return 0;
}

int sd_start(void)
{

    rt_thread_t tid;

    tid = rt_thread_create("sd_mount", sd_mount, RT_NULL,
                           1024, 9, 20);
    if (tid != RT_NULL)
    {
        rt_kprintf("enter sd_mount thread\r\n");
        rt_thread_startup(tid);
        return RT_EOK;
    }
    else
    {
        LOG_E("create sd_mount thread err!");
        return RT_ERROR;
    }

}


static int rt_hw_spi1_tfcard(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE );
    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_Pin_4);
    rt_kprintf(" attach spi device\r\n");
    return msd_init("sd0","spi10");
}


INIT_DEVICE_EXPORT(rt_hw_spi1_tfcard);

MSH_CMD_EXPORT(led,  led sample by using I/O drivers);
MSH_CMD_EXPORT(sd_start,  start sd thread);


