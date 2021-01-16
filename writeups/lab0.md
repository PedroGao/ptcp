# Lab 0 Writeup

## 重点

- 使用 `Buffer` 和 `BufferList` 来加速 `ByteStream` 的读写。
- `string_view` 的使用，本质上是一个可读的字符串，数据结构是 **[begin, size]**。
- 可写的字节是通过 `remain_capacity` 来计算，而可读的字节是通过`buffer_size` 来计算。
- `eof` 的条件是`输入结束(input_ended)`且`buffer_empty`，即写的没有了，而且读的也没有了。
