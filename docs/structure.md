# 代码组织结构阅读笔记

在实现 sponge TCP 的过程中，发现其实代码量很小，且逻辑也不难，主要是对 TCP 和 整个项目的理解。

切记在实现 lab 之前，要详细阅读代码的结构和基础类库，很重要！很重要！很重要！

```bash
libsponge/
├── tcp_helpers // TCP 帮助库
│   ├── tcp_header.cc
│   ├── tcp_header.hh // TCP 头部序列化，定义
│   ├── tcp_segment.cc
│   ├── tcp_segment.hh // TCP 包的解析，校验等
│   ├── tcp_state.cc
│   └── tcp_state.hh // TCP 状态机定义，分为接受方和发送方
├── util // 基础类库
│   ├── address.cc
│   ├── address.hh // 封装地址
│   ├── buffer.cc
│   ├── buffer.hh // 缓冲区
│   ├── eventloop.cc
│   ├── eventloop.hh // 事件监听器， poll 实现
│   ├── file_descriptor.cc
│   ├── file_descriptor.hh // 封装文件描述符
│   ├── parser.cc
│   ├── parser.hh // 解析器，主要解析 TCP 头部和 payload
│   ├── socket.cc
│   ├── socket.hh // 封装 socket
│   ├── tun.cc
│   ├── tun.hh // tun 文件描述符，虚拟句柄
│   ├── util.cc
│   └── util.hh // 基础类库，系统调用，校验和，异常类，16 进制序列化
```