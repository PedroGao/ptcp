# TCP 状态机

通过 google 的开源工具 packetdrill 可以观测到 linux 平台下
的网络包。

## SYN-SENT

观察 SYN-SENT 状态，SYN-SENT 是客户端的一个状态，当客户端通过 `connect` 连接服务端时，会发送一个携带 `ISN` 的 `SYN` 报文，只要服务端没有给出 `ACK` 回复，则客户端一直处于 SYN-SENT 的状态，并重发 SYN 包。

测试脚本如下：

``` sh
// 新建一个 socket
+0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
// 客户端 connect，此时会向对端发送一个 SYN 报文，进入 SYN-SENT 状态
+0 connect(3, ..., ...) = -1
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      1 192.168.16.17:50644     192.0.2.1:8080          SYN_SENT    - 
```

## SYN-RCVD

服务端在受到 `SYN` 报文后会回复客户端 `SYN+ACK` 报文，此时服务端会等待客户端的 `ACK` 报文，并进入 `SYN-RCVD` 的一个状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0
+0 setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
// 故意注释掉下面这一行
// +.1 < . 1:1(0) ack 1 win 65535

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.160.6:8080      0.0.0.0:*               LISTEN     -                   
tcp        0      0 192.168.160.6:8080      192.0.2.1:39309         SYN_RECV    -
```

## ESTABLISHED

双方三次握手成功后，服务端继续 `LISTEN` 状态，而客户端进入 `ESTABLISHED` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0
+0 setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
+.1 < . 1:1(0) ack 1 win 65535

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        1      0 192.168.110.162:8080    0.0.0.0:*               LISTEN      -                   
tcp        0      0 192.168.110.162:8080    192.0.2.1:54093         ESTABLISHED -
```

## FIN-WAIT-1

主动关闭连接的一方发送 `FIN` 包，等待对端 ACK 回复，此时进入 `FIN-WAIT-1` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
.1 < . 1:1(0) ack 1 win 65535

+.1 accept(3, ..., ...) = 4

+.1 close(4) = 0

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.60.182:8080     0.0.0.0:*               LISTEN      -                   
tcp        0      1 192.168.60.182:8080     192.0.2.1:59633         FIN_WAIT1   -
```

## FIN-WAIT-2

主动关闭方发送 `FIN` 包给对端，被对端确认后，并受到 `ACK` 包后，进入 `FIN-WAIT-2` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
// 故意注释掉下面这一行
.1 < . 1:1(0) ack 1 win 65535

+.1  accept(3, ..., ...) = 4

// 服务端主动断开连接
+.1 close(4) = 0

// 向协议栈注入 ACK 包，模拟客户端发送了 ACK
+.1 < . 1:1(0) ack 2 win 257

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.41.94:8080      0.0.0.0:*               LISTEN      -                   
tcp        0      0 192.168.41.94:8080      192.0.2.1:53723         FIN_WAIT2   - 
```

## CLOSE-WAIT

受到主动关闭方的 `FIN` 包，我方（被动方）进入 `CLOSE-WAIT` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
// 故意注释掉下面这一行
.1 < . 1:1(0) ack 1 win 65535

+.1  accept(3, ..., ...) = 4

// 向协议栈注入 FIN 包，模拟客户端发送了 FIN，主动关闭连接
+.1 < F. 1:1(0) win 65535  <mss 100> 
// 预期协议栈会发出 ACK
+0 > . 1:1(0) ack 2 <...> 
+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.39.137:8080     0.0.0.0:*               LISTEN      -                   
tcp        1      0 192.168.39.137:8080     192.0.2.1:37689         CLOSE_WAIT  - 
```

## TIME-WAIT

主动关闭方受到了被动关闭方的 `FIN` 包，因此发送 ACK 给对端，会开启 2MSL 的定时器，等待对端的 ACK 包，此时主动关闭方处于 `TIME-WAIT` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
.1 < . 1:1(0) ack 1 win 65535

+.1  accept(3, ..., ...) = 4

// 服务端主动断开连接
+.1 close(4) = 0
+0 > F. 1:1(0) ack 1 <...>

// 向协议栈注入 ACK 包，模拟客户端发送了 ACK
+.1 < . 1:1(0) ack 2 win 257

// 向协议栈注入 FIN，模拟服务端收到了 FIN
+.1 < F. 1:1(0) win 65535  <mss 100> 

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.128.46:8080     0.0.0.0:*               LISTEN      -                   
tcp        0      0 192.168.128.46:8080     192.0.2.1:38567         TIME_WAIT   -
```

## LAST-ACK

被动关闭方，在发送 `FIN` 包给对端后，等待对端的 ACK 报文，此时进入 `LAST-ACK` 状态。

测试脚本如下：

``` sh
--tolerance_usecs=1000000
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
+0 bind(3, ..., ...) = 0
+0 listen(3, 1) = 0

+0  < S 0:0(0) win 65535  <mss 100>
+0  > S. 0:0(0) ack 1 <...>
.1 < . 1:1(0) ack 1 win 65535

+.1  accept(3, ..., ...) = 4

// 向协议栈注入 FIN 包，模拟客户端发送了 FIN，主动关闭连接
+.1 < F. 1:1(0) win 65535  <mss 100> 
// 预期协议栈会发出 ACK
+0 > . 1:1(0) ack 2 <...> 

+.1 close(4) = 0
// 预期服务端会发出 FIN
+0 > F. 1:1(0) ack 2 <...> 

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.75.88:8080      0.0.0.0:*               LISTEN      -                   
tcp        1      1 192.168.75.88:8080      192.0.2.1:34431         LAST_ACK    -
```

## CLOSING

CLOSING 会在两端同时关闭的情况下出现，这里的同时不是时间意义上的同时，而是双方都发送的 `FIN` 包，但是都没有收到对端的 `ACK` 包。

测试脚本如下：

``` sh
--tolerance_usecs=100000
0.000 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
0.000 setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
0.000 bind(3, ..., ...) = 0
0.000 listen(3, 1) = 0

+0 < S 0:0(0) win 65535 <mss 1460>
+0 > S. 0:0(0) ack 1 <...>
+0.1 < . 1:1(0) ack 1 win 65535
0.200 accept(3, ..., ...) = 4

// 服务端随便传输一点数据给客户端
+0.100 write(4, ..., 1000) = 1000
// 断言服务端会发出 1000 字节的数据
+0 > P. 1:1001(1000) ack 1 <...>

// 确认 1000 字节数据
+0.01 < . 1:1(0) ack 1001 win 257

// 服务端主动断开，会发送 FIN 给客户端
+.1 close(4) = 0
// 断言协议栈会发出 ACK 确认（服务端->客户端）
+0 > F. 1001:1001(0) ack 1 <...>

// 客户端在未对服务端的 FIN 做确认时，也发出 FIN 要求断开连接
+.1 < F. 1:1(0) ack 1001 win 257

// 断言协议栈会发出 ACK 确认（服务端->客户端）
+0 > . 1002:1002(0) ack 2 <...>

// 故意不回 ACK，让连接处于 CLOSING 状态
// +.100 < . 2:2(0) ack 1002 win 257

+0 `sleep 1000000`
```

通过 `netstat` 观测的结果如下：

``` sh
tcp        0      0 192.168.39.231:8080     0.0.0.0:*               LISTEN      -                   
tcp        1      1 192.168.39.231:8080     192.0.2.1:43555         CLOSING     -
```
