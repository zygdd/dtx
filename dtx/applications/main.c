/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-12-09     RT-Thread    first version
 */

#include <rtthread.h>
#include <fal.h>
#include "usb_ota.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    fal_init();
    PD2_GPIO_Init();
    ota_usb_init();
    ota_usb_factory_init();
    LOG_D("123456789");
    int count = 5;



    while (count--)
    {

        LOG_D("Hello RT-Thread!");
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}

//修改APP分区地址的函数，不能删除
/**
 * Function    ota_app_vtor_reconfig
 * Description Set Vector Table base location to the start addr of app(RT_APP_PART_ADDR).
*/
static int ota_app_vtor_reconfig(void)
{
    #define RT_APP_PART_ADDR 0x08020000   // app运行的起始地址，根据自己的实际情况定义
    #define NVIC_VTOR_MASK   0x3FFFFF80
    /* Set the Vector Table base location by user application firmware definition */
    SCB->VTOR = RT_APP_PART_ADDR & NVIC_VTOR_MASK;

    return 0;
}
INIT_BOARD_EXPORT(ota_app_vtor_reconfig);


#include <unistd.h>
#include <fcntl.h>
#define TEST_FN     "/test_usbh.c"
static char test_data[120], buffer[120];

void readwrite(const char* filename)
{
    int fd;
    int index, length;

    fd = open(TEST_FN, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        rt_kprintf("open file for write failed\n");
        return;
    }

    for (index = 0; index < sizeof(test_data); index ++)
    {
        test_data[index] = index + 27;
    }

    length = write(fd, test_data, sizeof(test_data));
    if (length != sizeof(test_data))
    {
        rt_kprintf("write data failed\n");
        close(fd);
        return;
    }

    close(fd);

    fd = open(TEST_FN, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("check: open file for read failed\n");
        return;
    }

    length = read(fd, buffer, sizeof(buffer));
    if (length != sizeof(buffer))
    {
        rt_kprintf("check: read file failed\n");
        close(fd);
        return;
    }

    for (index = 0; index < sizeof(test_data); index ++)
    {
        if (test_data[index] != buffer[index])
        {
            rt_kprintf("check: check data failed at %d\n", index);
            close(fd);
            return;
        }
    }

    rt_kprintf("usb host read/write udisk successful\r\n");

    close(fd);
}

MSH_CMD_EXPORT(readwrite, usb host read write test);




