# xnet简介
xnet是用c+lua编写的跨平台（支持windows和linux）网络库，c层网络库可以单独使用，也内置了lua的支持。xnet的主要设计目标是节省资源、使用方便。xnet占用资源非常小，适用于嵌入式场景。

一般情况下推荐用lua的方式来使用。

## 方便的注册机制
例如，xnet可以通过注册函数的方式来处理网络io、定时器事件：

```lua
xnet.register({
	listen = function(sid, ns, addr)
		--连接到达
	end,
	error = function(sid, what)
		--连接断开、关闭
	end,
	recv = function(sid, pkg_type, pkg, sz, addr)
		--接收数据
	end,
	timeout = function(id)
		--超时时间处理
	end,
	command = function(source, command, data, sz)
		--内部命令
	end,
	connected = function(sid, err)
		--连接回调
	end,
})
```

### packer机制

可以给连接注册解包方法，例如注册http解包方法，只需要在listen事件中给连接注册解包方式即可，内置了3种解包方式，分别是：http、sizebuffer(4字节长度+实际内容)、line（单行文本）。例如注册一个http解包方法，可以这样：
```
listen = function(sid, ns, addr)
	xnet.register_packer(ns, xnet.PACKER_TYPE_HTTP)
end,
```
同时，lualib/pack.lua中也提供了相应的封包方法，详细可以查看该代码文件以及luaexample。

## xnet配置
启动xnet需要提供一个配置文件，配置文件示例如下：
```
log_path = "./run.log"  #日志路径
luabooter = "./luaexample/main.lua"  #lua入口文件
disable_log_thread = true #禁用日志线程，直接在主线程中输出
```

如果不配置log_path的话，日志会默认输出到控制台。luabooter用于配置lua的入口文件，如果不配置luabooter的话，会尝试读xnet运行路径下的main.lua。disable_log_thread默认为false，即不禁用，如果你想节省一些资源或者只想单线程运行的话，可以禁用日志线程。

在lua中也可以通过xnet.get_env来获取配置信息。

## lua入口文件的默认方法
lua入口文件必须声明Start、Init、Stop函数，用于流程控制。服务器启动时，会先调用Start，然后调用Init，服务器关闭时，会调用Stop。如果没有声明，启动会报错。

## 编译lua
**Linux:**

```shell
cd 3rd/lua
make linux
```

**Windows(for MinGW):**

```shell
cd 3rd/lua
make mingw
```

## 编译xnet
**Linux:**

```shell
make linux
```

**Windows(for MinGW):**

```shell
make win
```

## 运行lua示例

```shell
./xnet.exe xnet.config
```

## 设计相关
1. windows下使用select，linux下使用epoll。
2. socket大部分跨平台代码在xnet_socket中实现。
3. 通过注册回调函数的方式处理socket io事件。
4. 可以通过注册command回调处理用户自定义事件。
5. 提供pack/unpack机制（目前支持http、sizebuffer、line），可以对数据方便地进行处理。
6. 提供了简单的异步日志实现。
7. 支持udp，ipv6。