/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     SummerGift   add spi flash port file
 */

#include <rtthread.h>
#include "spi_flash.h"
#include "spi_flash_sfud.h"
#include "drv_spi.h"


static int rt_hw_spi_flash_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();                                   //起到将设备挂载到spi总线的作用
    rt_hw_spi_device_attach("spi2", "spi20", GPIOB, GPIO_PIN_12);   //参数分别是：spi总线号、spi设备名、片选引脚的gpio组、片选引脚的gpio端口号

    if (RT_NULL == rt_sfud_flash_probe("W25Q128", "spi20"))         //这一步是挂载flash，参数对应：自定义的设备名称、对应的spi设备。
    {
        return -RT_ERROR;
    };

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);


