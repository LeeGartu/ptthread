# 背景

开发基于rtthread的线控器产品，为了提高开发效率，在电脑上模拟底层硬件的api实现，使用PC模拟产品运行。
关于为什么不用官方的bsp里面的simulator？其实一开始也尝试过，但是x64编译问题一大堆报错，于是就诞生了这个项目。

## 技术路线

1. rtthread 使用 4.1.1 版本
2. 使用 pthread 模拟 rtthread 的线程创建。实现 api 兼容。
3. 使用 libserialport 模拟 串口硬件中断。
4. 使用 time.h 模拟 rtthread 的定时器，存在一点误差。

### components
1. drivers 只保留 ipc和serial这两个组件。
2. utilites 只保留 ulog和utest这两个组件。

### 线程调度器 （完全删除）
取消了线程挂起和恢复的特性。
使用信号量、互斥量等IPC机制解决并发与竞争问题。

## 已实现

### 线程创建
1. 'rt_thread_create'
2. 'rt_thread_startup'
3. 'rt_components_init' 使用 __attribute__((constructor)) 实现。所以需要单独添加对应的c文件。target_sources

### 串口驱动 （V1）
1. 'rt_device_open'
2. 'rt_device_set_rx_ind'
3. 'rt_hw_serial_register'
4. 'rt_serial_read'
5. 'rt_serial_write'

### tick和timer
1. 使用time.h模拟

### IPC模拟
1. sem
2. mutex
3. event
4. mailbox
5. mq
