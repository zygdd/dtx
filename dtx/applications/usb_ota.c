/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-18     86188       the first version
 */
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-19     86188       the first version
 */
#include <rtthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <finsh.h>
#include <fal.h>
#include <unistd.h>
#include <fcntl.h>

#define DBG_SECTION_NAME          "ota_usb"

#define DBG_LEVEL                 DBG_LOG

#define DBG_COLOR
#include <rtdbg.h>

/* 固件版本号 */
//#define APP_VERSION               "1.0.0"

/* 固件名称 */
#define USBH_UPDATE_FN            "/rtthread.rbl"

/* 固件下载分区名称 */
#define DEFAULT_DOWNLOAD_PART     "download"    //  download

static char* recv_partition = DEFAULT_DOWNLOAD_PART;

rt_sem_t ota_sem = RT_NULL;

static void print_progress(size_t cur_size, size_t total_size)
{
    static unsigned char progress_sign[100 + 1];
    uint8_t i, per = cur_size * 100 / total_size;

    if (per > 100)
    {
        per = 100;
    }

    for (i = 0; i < 100; i++)
    {
        if (i < per)
        {
            progress_sign[i] = '=';
        }
        else if (per == i)
        {
            progress_sign[i] = '>';
        }
        else
        {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    LOG_I("\033[2A");
    LOG_I("Download: [%s] %d%%", progress_sign, per);
}

static void ota_usb_download_entry(void* parameter)
{
    //DIR *dirp;
    static size_t update_file_total_size, update_file_cur_size;
    static const struct fal_partition * dl_part = RT_NULL;
    static int fd;
    static rt_uint8_t buf[1024];
    static rt_err_t result;
    struct stat file;

    //rt_kprintf("The current version of APP firmware is %s\n", APP_VERSION);

    while(1)
    {
        result = rt_sem_take(ota_sem, RT_WAITING_FOREVER);
        if(result == RT_EOK)
        {
            rt_kprintf("Default save firmware on download partition.\n");

            rt_kprintf("Warning: usb has started! This operator will not recovery.\n");

            /* 查询固件大小 */
            result = stat(USBH_UPDATE_FN, &file);
            if(result == RT_EOK)
            {
                LOG_D(""USBH_UPDATE_FN" file size is : %d\n", file.st_size);
            }
            else
            {
                LOG_E(""USBH_UPDATE_FN" file not fonud.");
                goto __exit;
            }
            if(file.st_size <= 0)
            {
                LOG_E(""USBH_UPDATE_FN" file is empty.");
                goto __exit;
            }

            /* 获取"download"分区信息并清除分区数据 */
            if ((dl_part = fal_partition_find(recv_partition)) == RT_NULL)
            {
                LOG_E("Firmware download failed! Partition (%s) find error!", recv_partition);
                goto __exit;
            }
            if (file.st_size > dl_part->len)
            {
                LOG_E("Firmware is too large! File size (%d), '%s' partition size (%d)", file.st_size, recv_partition, dl_part->len);
            }
            if (fal_partition_erase(dl_part, 0, file.st_size) < 0)
            {
                LOG_E("Firmware download failed! Partition (%s) erase error!", dl_part->name);
            }

            /* 以只读模式打开固件 */
            fd = open(USBH_UPDATE_FN, O_RDONLY);
            if (fd >= 0)
            {
                do
                {
                    /* 读取u盘的固件，一次读1024字节 */
                    update_file_cur_size = read(fd, buf, sizeof(buf));

                    /* 把固件写入片外flash的download分区  */
                    if (fal_partition_write(dl_part, update_file_total_size, buf, update_file_cur_size) < 0)
                    {
                        LOG_E("Firmware download failed! Partition (%s) write data error!", dl_part->name);
                        close(fd);
                        goto __exit;
                    }

                    update_file_total_size += update_file_cur_size;

                    print_progress(update_file_total_size, file.st_size);
                }
                while (update_file_total_size != file.st_size);

                close(fd);
            }
            else
            {
                LOG_E("check: open file for read failed\n");
                goto __exit;
            }

            rt_kprintf("Download firmware to flash success.\n");
            rt_kprintf("System now will restart...\r\n");

            /* wait some time for terminal response finish */
            rt_thread_delay(rt_tick_from_millisecond(200));

            /* Reset the device, Start new firmware */
            extern void rt_hw_cpu_reset(void);
            rt_hw_cpu_reset();
            /* wait some time for terminal response finish */
            rt_thread_delay(rt_tick_from_millisecond(200));

        __exit:
            LOG_E("download rtthread.rbl file failed");
        }
    }
}

void ota_usb_init(void)
{
    fal_init();

    ota_sem = rt_sem_create("otasem", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t thread1 = rt_thread_create("ota_usb", ota_usb_download_entry, RT_NULL, 4096, 26, 10);
    if (thread1 != RT_NULL)
    {
        rt_thread_startup(thread1);
    }
}
//INIT_APP_EXPORT(ota_usb_init);


void PD2_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}


static rt_uint8_t ota_usb_start(void)
{
    rt_sem_release(ota_sem);

    return RT_EOK;
}
MSH_CMD_EXPORT(ota_usb_start, OTA start);

