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


#define DBG_SECTION_NAME          "ota_usb_factory"

#define DBG_LEVEL                 DBG_LOG

#define DBG_COLOR
#include <rtdbg.h>

/* 固件版本号 */
//#define APP_VERSION_factory               "0.0.0"

/* 固件名称 */
#define USBH_UPDATE_FN_factory            "/factory/rtthread.rbl"

/* 固件下载分区名称 */
#define DEFAULT_DOWNLOAD_PART_factory     "factory"

static char* recv_partition_factory = DEFAULT_DOWNLOAD_PART_factory;

rt_sem_t ota_sem_factory = RT_NULL;

static void print_progress_factory(size_t cur_size, size_t total_size)
{
    static unsigned char progress_sign_factory[100 + 1];
    uint8_t i_factory, per_factory = cur_size * 100 / total_size;

    if (per_factory > 100)
    {
        per_factory = 100;
    }

    for (i_factory = 0; i_factory < 100; i_factory++)
    {
        if (i_factory < per_factory)
        {
            progress_sign_factory[i_factory] = '=';
        }
        else if (per_factory == i_factory)
        {
            progress_sign_factory[i_factory] = '>';
        }
        else
        {
            progress_sign_factory[i_factory] = ' ';
        }
    }

    progress_sign_factory[sizeof(progress_sign_factory) - 1] = '\0';

    LOG_I("\033[2A");
    LOG_I("Factory: [%s] %d%%", progress_sign_factory, per_factory);
}

static void ota_usb_factory_entry(void* parameter)
{
    //DIR *dirp;
    static size_t update_file_total_size_factory, update_file_cur_size_factory;
    static const struct fal_partition * dl_part_factory = RT_NULL;
    static int fd_factory;
    static rt_uint8_t buf_factory[1024];
    static rt_err_t result_factory;
    struct stat file_factory;

    //rt_kprintf("The current version of factory firmware is %s\n", APP_VERSION);

    while(1)
    {
        result_factory = rt_sem_take(ota_sem_factory, RT_WAITING_FOREVER);
        if(result_factory == RT_EOK)
        {
            rt_kprintf("Default save firmware on factory partition.\n");

            rt_kprintf("Warning: usb has started! This operator will not recovery.\n");

            /* 查询固件大小 */
            result_factory = stat(USBH_UPDATE_FN_factory, &file_factory);
            if(result_factory == RT_EOK)
            {
                LOG_D(""USBH_UPDATE_FN_factory" file_factory size is : %d\n", file_factory.st_size);
            }
            else
            {
                LOG_E(""USBH_UPDATE_FN_factory" file_factory not fonud.");
                goto __exit;
            }
            if(file_factory.st_size <= 0)
            {
                LOG_E(""USBH_UPDATE_FN_factory" file_factory is empty.");
                goto __exit;
            }

            /* 获取"factory"分区信息并清除分区数据 */
            if ((dl_part_factory = fal_partition_find(recv_partition_factory)) == RT_NULL)
            {
                LOG_E("Firmware factory failed! Partition (%s) find error!", recv_partition_factory);
                goto __exit;
            }
            if (file_factory.st_size > dl_part_factory->len)
            {
                LOG_E("Firmware is too large! File size (%d), '%s' partition size (%d)", file_factory.st_size, recv_partition_factory, dl_part_factory->len);
            }
            if (fal_partition_erase(dl_part_factory, 0, file_factory.st_size) < 0)
            {
                LOG_E("Firmware factory failed! Partition (%s) erase error!", dl_part_factory->name);
            }

            /* 以只读模式打开固件 */
            fd_factory = open(USBH_UPDATE_FN_factory, O_RDONLY);
            if (fd_factory >= 0)
            {
                do
                {
                    /* 读取u盘的固件，一次读1024字节 */
                    update_file_cur_size_factory = read(fd_factory, buf_factory, sizeof(buf_factory));

                    /* 把固件写入片外flash的download分区  */
                    if (fal_partition_write(dl_part_factory, update_file_total_size_factory, buf_factory, update_file_cur_size_factory) < 0)
                    {
                        LOG_E("Firmware factory failed! Partition (%s) write data error!", dl_part_factory->name);
                        close(fd_factory);
                        goto __exit;
                    }

                    update_file_total_size_factory += update_file_cur_size_factory;

                    print_progress_factory(update_file_total_size_factory, file_factory.st_size);
                }
                while (update_file_total_size_factory != file_factory.st_size);

                close(fd_factory);
            }
            else
            {
                LOG_E("check: open file for read failed\n");
                goto __exit;
            }

            rt_kprintf("Factory firmware to flash success.\n");
            rt_kprintf("System now will restart...\r\n");

            /* wait some time for terminal response finish */
            rt_thread_delay(rt_tick_from_millisecond(200));

            /* Reset the device, Start new firmware */
            extern void rt_hw_cpu_reset(void);
            rt_hw_cpu_reset();
            /* wait some time for terminal response finish */
            rt_thread_delay(rt_tick_from_millisecond(200));

        __exit:
            LOG_E("Factory rtthread.rbl file failed");
        }
    }
}

void ota_usb_factory_init(void)
{
    fal_init();

    ota_sem_factory = rt_sem_create("otasem_factory", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t thread1_factory = rt_thread_create("ota_usb_factory", ota_usb_factory_entry, RT_NULL, 4096, 26, 10);
    if (thread1_factory != RT_NULL)
    {
        rt_thread_startup(thread1_factory);
    }
}

static rt_uint8_t ota_usb_factory_start(void)
{
    rt_sem_release(ota_sem_factory);

    return RT_EOK;
}
MSH_CMD_EXPORT(ota_usb_factory_start, OTA_factory start);





