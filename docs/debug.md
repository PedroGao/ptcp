# debug

关于 lab4，一直有一些问题，如下：

```sh
        Start  57: t_ucS_1M_32k
 55/162 Test  #57: t_ucS_1M_32k .....................***Failed    6.37 sec
DEBUG: Listening for incoming connection... DEBUG: Connecting to 169.254.144.1:24885... done.
new connection from 169.254.144.1:34015.
DEBUG: Outbound stream to 169.254.144.1:34015 finished (1 byte still in flight).
DEBUG: Inbound stream from 169.254.144.1:24885 finished cleanly.
DEBUG: Outbound stream to 169.254.144.1:34015 has been fully acknowledged.
DEBUG: Waiting for clean shutdown... DEBUG: Outbound stream to 169.254.144.1:24885 finished (1080633 bytes still in flight).
DEBUG: Inbound stream from 169.254.144.1:34015 finished cleanly.
DEBUG: Waiting for lingering segments (e.g. retransmissions of FIN) from peer...
DEBUG: Waiting for clean shutdown... DEBUG: TCP connection finished uncleanlydone.
ERROR: 5d8abc679f5a2cb1b1052f000b0bba101994a53bb47e25b85f441decee2a6687 neq 06cc8d4d84e9e1fde9f447a96649fdef789e6b747033c405e5523bddbec12702 or
```

`u` 是选择 UDP 模式，`c`是选择 client，`S` 是 Send Test，