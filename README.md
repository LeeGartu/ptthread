# 背景

开发基于rtthread的线控器产品，为了提高开发效率，在电脑上模拟底层硬件的api实现，使用PC模拟产品运行。
关于为什么不用官方的bsp里面的simulator？其实一开始也尝试过，但是x64编译问题一大堆报错，于是就诞生了这个项目。

## 技术路线

1. 使用 pthread 模拟 rtthread 的线程创建。实现 api 兼容。
2. 使用 libserialport 模拟 串口硬件中断。

## 已实现

### 线程调度器 （完全删除）
取消了线程挂起和恢复的特性。
使用信号量、互斥量等IPC机制实现线程同步。

### 线程创建
1. 'rt_thread_create'
2. 'rt_thread_startup'
3. 'rt_components_init' 使用 __attribute__((constructor)) 实现。所以需要单独添加对应的c文件。target_sources

### 串口驱动
1. 'rt_device_open'
2. 'rt_device_set_rx_ind'
3. 'rt_hw_serial_register'
4. 'rt_serial_read'
5. 'rt_serial_write'
