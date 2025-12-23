/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-13     bernard      first version
 * 2012-05-15     lgnq         modified according bernard's implementation.
 * 2012-05-28     bernard      code cleanup
 * 2012-11-23     bernard      fix compiler warning.
 * 2013-02-20     bernard      use RT_SERIAL_RB_BUFSZ to define
 *                             the size of ring buffer.
 * 2014-07-10     bernard      rewrite serial framework
 * 2014-12-31     bernard      use open_flag for poll_tx stream mode.
 * 2015-05-19     Quintin      fix DMA tx mod tx_dma->activated flag !=RT_FALSE BUG
 *                             in open function.
 * 2015-11-10     bernard      fix the poll rx issue when there is no data.
 * 2016-05-10     armink       add fifo mode to DMA rx when serial->config.bufsz != 0.
 * 2017-01-19     aubr.cool    prevent change serial rx bufsz when serial is opened.
 * 2017-11-07     JasonJia     fix data bits error issue when using tcsetattr.
 * 2017-11-15     JasonJia     fix poll rx issue when data is full.
 *                             add TCFLSH and FIONREAD support.
 * 2018-12-08     Ernest Chen  add DMA choice
 * 2020-09-14     WillianChan  add a line feed to the carriage return character
 *                             when using interrupt tx
 * 2020-12-14     Meco Man     implement function of setting window's size(TIOCSWINSZ)
 * 2021-08-22     Meco Man     implement function of getting window's size(TIOCGWINSZ)
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "libserialport.h"

#define DBG_TAG    "UART"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

// 全局控制变量（实际项目中建议封装）
thread_control_t ctrl = {
    .paused = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond  = PTHREAD_COND_INITIALIZER
};
#define THREAD_DEV_NUM 5
rt_device_t thread_dev[THREAD_DEV_NUM] = {NULL};
int thread_dev_idx = 0;

// 主线程：控制暂停/恢复
void pause_thread(thread_control_t *ctrl) {
    pthread_mutex_lock(&ctrl->mutex);
    ctrl->paused = true;
    pthread_mutex_unlock(&ctrl->mutex);
}

void resume_thread(thread_control_t *ctrl) {
    pthread_mutex_lock(&ctrl->mutex);
    ctrl->paused = false;
    pthread_cond_broadcast(&ctrl->cond); // 唤醒等待的线程
    pthread_mutex_unlock(&ctrl->mutex);
}

static void *rx_polling_thread(void *param)
{
    rt_list_t *node;
    while(1) {
        // 检查是否需要暂停
        pthread_mutex_lock(&ctrl.mutex);
        while (ctrl.paused) {
            pthread_cond_wait(&ctrl.cond, &ctrl.mutex); // 阻塞等待恢复
        }
        for (int i = 0; i < thread_dev_idx; i++) {
            rt_device_t dev = thread_dev[i];
            struct sp_port *port = dev->user_data;

            int n = sp_input_waiting(port);
            if ((n > 0) && dev->rx_indicate) {
                dev->rx_indicate(dev, n);
            }
        }

        pthread_mutex_unlock(&ctrl.mutex);
    }
}

int rx_polling_add_dev_list(rt_device_t dev)
{
    pause_thread(&ctrl);
    if (thread_dev[0] == NULL) {
        pthread_t tid;
        pthread_create(&tid, NULL, rx_polling_thread, NULL);
        pthread_detach(tid);
    }
    thread_dev[thread_dev_idx] = dev;
    thread_dev_idx++;
    resume_thread(&ctrl);

    return 0;
}

/* RT-Thread Device Interface */
/*
 * This function initializes serial device.
 */
static rt_err_t rt_serial_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    /* initialize rx/tx */
    serial->serial_rx = RT_NULL;
    serial->serial_tx = RT_NULL;

    /* apply configuration */
    if (serial->ops->configure)
        result = serial->ops->configure(serial, &serial->config);

    return result;
}

static rt_err_t rt_serial_open(struct rt_device *dev, rt_uint16_t oflag)
{
    rt_uint16_t stream_flag = 0;
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;
    
    return rx_polling_add_dev_list(dev);
}

static rt_err_t rt_serial_close(struct rt_device *dev)
{
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    /* this device has more reference count */
    if (dev->ref_count > 1) return RT_EOK;

    return RT_EOK;
}

static rt_size_t rt_serial_read(struct rt_device *dev,
                                rt_off_t          pos,
                                void             *buffer,
                                rt_size_t         size)
{
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    if (size == 0) return 0;

    serial = (struct rt_serial_device *)dev;
    struct sp_port *port = dev->user_data;

    return sp_blocking_read(port, buffer, size, 0);
}

static rt_size_t rt_serial_write(struct rt_device *dev,
                                 rt_off_t          pos,
                                 const void       *buffer,
                                 rt_size_t         size)
{
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    if (size == 0) return 0;

    serial = (struct rt_serial_device *)dev;
    struct sp_port *port = dev->user_data;

    sp_blocking_write(port, buffer, size, 0);
    return size;
}

static rt_err_t rt_serial_control(struct rt_device *dev,
                                  int              cmd,
                                  void             *args)
{
    rt_err_t ret = RT_EOK;
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    switch (cmd)
    {
        case RT_DEVICE_CTRL_SUSPEND:
            /* suspend device */
            dev->flag |= RT_DEVICE_FLAG_SUSPENDED;
            break;

        case RT_DEVICE_CTRL_RESUME:
            /* resume device */
            dev->flag &= ~RT_DEVICE_FLAG_SUSPENDED;
            break;

        case RT_DEVICE_CTRL_CONFIG:
            if (args)
            {
                struct serial_configure *pconfig = (struct serial_configure *) args;
                if (pconfig->bufsz != serial->config.bufsz && serial->parent.ref_count)
                {
                    /*can not change buffer size*/
                    return RT_EBUSY;
                }
                /* set serial configure */
                serial->config = *pconfig;
                if (serial->parent.ref_count)
                {
                    /* serial device has been opened, to configure it */
                    serial->ops->configure(serial, (struct serial_configure *) args);
                }
            }

            break;
        default :
            /* control device */
            ret = serial->ops->control(serial, cmd, args);
            break;
    }

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops serial_ops =
{
    rt_serial_init,
    rt_serial_open,
    rt_serial_close,
    rt_serial_read,
    rt_serial_write,
    rt_serial_control
};
#endif

/*
 * serial register
 */
rt_err_t rt_hw_serial_register(struct rt_serial_device *serial,
                               const char              *name,
                               rt_uint32_t              flag,
                               void                    *data)
{
    rt_err_t ret;
    struct rt_device *device;
    RT_ASSERT(serial != RT_NULL);

    device = &(serial->parent);

    device->type        = RT_Device_Class_Char;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    device->ops         = &serial_ops;
#else
    device->init        = rt_serial_init;
    device->open        = rt_serial_open;
    device->close       = rt_serial_close;
    device->read        = rt_serial_read;
    device->write       = rt_serial_write;
    device->control     = rt_serial_control;
#endif
    device->user_data   = data;

    /* register a character device */
    ret = rt_device_register(device, name, flag);

    return ret;
}
