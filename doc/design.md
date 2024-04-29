socket listen
    socket | direct
    bind   | direct
    listen | select/epoll

大体参考libevent的设计
尽量将socket大部分跨平台代码在xnet_socket中实现

* 实现数据收发、监听
* 先只实现单线程模式
* 编写makefile，调试
* socket 改为异步，设置keepalive
* error改为跨平台
* 实现管道命令机制
* 实现connect
* 实现linux平台支持
* 定时机制

优化排版
多线程支持
半关闭机制（主动关闭前先推送完队列中的数据）

4.18
实现发数据功能（wb_list/xnet_send_data）
makefile初步编写，编译通过
4.19
telnet测试通过，完善了一些socket接口
4.22
实现管道命令机制
4.25
实现connect
实现linux平台支持
4.27
完善linux平台支持
4.28
完成timeheap
完成定时机制
完成win平台下定时机制的测试
4.29
1.完成linux平台下定时机制的测试