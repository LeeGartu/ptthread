背景：开发基于rtthread的线控器产品，为了提高开发效率，在电脑上模拟底层硬件的api实现，使用PC模拟产品运行。

技术路线：
    1. 使用 pthread 模拟 rtthread 的线程创建。实现 api 兼容。
    2. 使用 libserialport 模拟 串口硬件中断。

