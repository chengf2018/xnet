## finished
* 实现数据收发、监听 *
* 先只实现单线程模式 *
* 编写makefile，调试 *
* socket 改为异步，设置keepalive *
* error改为跨平台 *
* 实现管道命令机制 *
* 实现connect *
* 实现linux平台支持 *
* 定时机制 *
* 多线程支持 管道命令完善 *
* 半关闭机制（主动关闭前先推送完队列中的数据） *
* 异步日志 *
* context生命周期优化，ctx如果被释放？但是还有引用？ *
* 配置解析模块，server支持配置传参 *
* ipv6支持 *
* udp支持 *
* udp测试 *
* socket数据包多发优化，共用一个数据包 *
* pack/unpack机制 *
* 异步日志支持开关 *
* 优化socket slots 占用内存过大问题 *

## todo list

* 优化排版 *

## log
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

4.30
1.完成win下connect的测试
2.完善管道命令

5.6
1.管道命令完善

5.7
1.半关闭机制
2.连接、监听回调
3.context增加id
4.删除无用文件
5.异步日志功能

5.9
1.配置模块初步编写

5.10
1.完善配置模块

5.11
1.完成配置模块测试

5.13
server初始化配置、代码优化完善

5.20
完善ipv6支持，udp支持

5.21
1.完善udp数据收发
2.修复vsnprintf bug

5.23
1.完善udp，udp测试

5.24
1.socket数据包多发优化，共用一个数据包

6.7
1.完成http unpack，以及测试

6.8
1.完成简单的http server示例
2.多发接口优化

6.10
1.完成sizebuffer/linebuffer的pack/unpack

7.25
1.timeheap优化