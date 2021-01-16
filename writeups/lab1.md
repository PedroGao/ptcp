# Lab 1 Writeup

## Hints

- StreamReassembler 作用在接收方，重新组成 ByteStream 中的数据。
- 而发送方只需要在 ByteStream 里面写数据即可。
- StreamReassembler 存在的意义在于网络包可能丢失、重发或者各种不稳定的情况。
- StreamReassembler 接受 TcpSegment 然后按照序号重组，一旦确认了序号，则将 TcpSegment 数据写入到 ByteStream 中。
- 数据超过了 StreamReassembler 的容量会默认丢弃，不必警告，直接丢弃。

## 重点

- StreamReassembler 的 `capacity` 由两部分组成；1、ByteStream 的容量，即已重组的字节；2、已接受的字节，但未被重组。总之就是收到的字节数。
- 对于两个节点`node1`和`node2`的 `merge`,如果 node1 和 node2 刚好相邻，则 merge 的返回值为 0，因此 node1 和 node2 无交集必须返回负数。外层也必须根据`<0`来判断是否合并成功。
- 这里由于未重组的字节需要放在额外的存储空间中，考虑到`有序`和`快速查找`的两个特性，因此选择`红黑树`，即`set`。
- 对于 `set` 的 `lower_bound` 和 `upper_bound` 容易误解，`lower_bound` 返回大于等于当前 `key` 的指针，而
  `upper_bound`返回大于`key`的指针。如果需要向前合并，可以在`lower_bound`以后，指针`--`来得到前一个节点。
- 文档上虽然说明需要丢弃超过容量字节，但是判断后丢弃单侧不过，而不判断反而没有任何问题，我也很难受~
- `_head_index` 这个额外指针很重要，代表当前已经重组的字节量，`_head_index`默认为`0`，根据重组多少来增加。
