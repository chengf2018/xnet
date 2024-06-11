# xnet
xnet是用c编写的跨平台（支持windows和linux）网络库。

xnet具有以下特点：

1. windows下使用select，linux下使用epoll。
2. socket大部分跨平台代码在xnet_socket中实现。
3. 通过注册回调函数的方式处理socket io事件。
4. 可以通过注册command回调处理用户自定义事件。
5. 提供pack/unpack机制（目前支持http、sizebuffer、line），可以对数据方便地进行处理。
6. 提供了简单的异步日志实现。
7. 支持udp，ipv6。

## build
**Linux:**

```
make linux
```

**Windows(for MinGW):**

```
make win
```