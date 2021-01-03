# Lab 3 Writeup

## Hints

- TCPSender 负责将数据流包装成 TCPSegment 然后发送。
- Keep track of the receiver’s window (processing incoming acknos and window sizes)，处理 acknos 和滑动窗口。
- 暂时存储已发送但未被确认的包，称为`outstanding`包。
- 重发超时但未被确认的包。
- 使用 `tick` 函数去计时，不要使用其它的时间函数。
- 有包需要确认的时候，开启 `timer`，当所有包都被确认后，关闭 `timer`。

## 重点

- SYN 包不包含数据，直接发送确认即可。
- \_window_size 是 0 的时候，sender 将 \_window_size 当作 1 看待，即使这个包可能会被 receiver 拒绝，当时可以当作窗口信息的通信，
  则当接收方有空余的窗口接受其它的数据时，sender 再开始发送真正的数据。
- 发送方不需要去管发送包的 `ackno`。
- 记录连续的重传次数，在重传后增加，如果在单位时间内重传次数过多，那么可退出当前连接。
- 当`tick`的时候，如果 \_window_size 不为 0，则超时加倍，否则，不加倍。
- \_window_size 应该特殊处理，在初始化状态下不能是`0`，因为对端发送的`window_size`为 0 的话，此端需要特殊处理。
