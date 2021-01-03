# Lab 2 Writeup

## Hints

- 之前的 lab 全是关于字节流的接收和重组，从这个实验开始来实现 `tcp`，本节实验实现 `TCPReceiver`。
- `TCPReceiver` 处理进来(inbound)的字节，将 `TCPSegment` 包翻译成字节流。
- 通过 `void segment_received(const TCPSegment &seg);` 来接受来自网络的包，并将包数据交付给`StreamReassembler`重组器，然后将字节流写入`ByteStream`中。
- 64 位序号与 32 位`seqno` 之间的转换。每一个字节都有一个唯一的 64 位序号，且从`0`开始递增。

## seqno

在 TCP header 中，空间是有限的，因此不会使用`64`位的字节序号，64 位的字节序号通常可以认为是无限的，但是在 tcp header
中 seqno 是 `32` 位的，所以这里必须有一个转换，因此会有一些麻烦点。

1. seqno 是 32 位的，所以最多发送 4G 数据，这是不够的，因此当 seqno 增加到 `2^32 - 1`时，下一个字节序号就会变成`0`。
2. seqno 会随机以一个 32 位随机数开始，被称为`ISN`；第一个字节数据的 seqno 是 `ISN+1 (mod 2^32)`, 第二个字节数据的 seqno 是
   `ISN+2 (mod 2^32)`.
3. 字节流的起点和重点都将占有一个 seqno 序号，即 `SYN` 和 `FIN` 包都会消耗一个 seqno。

|    element     |   syn    |    c     |  a  |  t  | fin |
| :------------: | :------: | :------: | :-: | :-: | :-: |
|     seqno      | 2^32 − 2 | 2^32 − 1 |  0  |  1  |  2  |
| absolute seqno |    0     |    1     |  2  |  3  |  4  |
|  stream index  |          |    0     |  1  |  2  |     |

`seqno` 是 tcp header 中的序列号，因此是随机初始化的，这里 ISN 是 `2^32 - 2`，而 absolute seqno 是绝对
序号，因此是从 0 开始的，而 syn 和 fin 都会消耗一个序号，所以 syn 是 0，而 `c` 字节是 1，而 stream index
是数据的序号，因此 `c` 才是 0。

这里 absolute seqno 是 无包装的 64 位数字，而 seqno 是 32 位包装数字。

对于`unwrap`即`seqno`转`absolute seqno`，实际操作就是把算出来的 绝对序号分别加减 `1ul << 32` 做比较，选择与 checkpoing 差的绝对值最小的那个。

## 重点

- the index of the first unassembled byte, which is called the acknowledgment number
  or ackno. This is the first byte that the receiver needs from the sender。第一个未被重组的
  字节序列，被称为确认序列，即`ackno`，这是接受者第一个需要接收的字节。一定是第一个未被重组的字节序列，因为可能会存在
  丢包各种 case。
- the distance between the first unassembled index and the first unacceptable index.
  This is called the window size. 第一个未被重组的字节序列和第一个未被接收的字节序列的距离被称为滑动窗口距离。
- `segment_received` 中的细节很多，关键在于如何权衡`abs_seqno`，`seqno`，`syn` 和 `fin` 之间的关系，注意包是可以
  丢失，重传和篡改的，所以本质上是无须的，因此一定要谨慎判断条件。
