## lab2

## 注意点

* 在 TCP 中， SYN(开始) 和 FIN(结束) 同样消耗序号。
* TCP 序号不是从零开始。

|  element |   SYN    | c | a | t | FIN |
|  ------  | -------- |---|---|---| --- |
| seqno    | 2^32 - 2 | 2^32 -1  | 0 | 1 | 2|
| absolute seqno | 0 | 1  | 2  | 3  | 4  |
| stream index  |  | 0 | 1 | 2  |  | 

Note: A checkpoint is required because any seqno corresponds to many absolute
seqnos. E.g. the seqno “17” corresponds to the absolute seqno of 17, but also 2^32 + 17, 
or 2^33 + 17, or 2^34 + 17, etc. The checkpoint helps to resolve the ambiguity: it’s a
recently unwrapped absolute seqno that the user of this class knows is close to the
desired unwrapped absolute seqno for this seqno. In your TCP implementation, you’ll
use the index of the last reassembled byte as the checkpoint.

注意： checkpoint 是必须的，因为一个 seqno 可以关联到多个 absolute seqnos。例如，seqno `17` 可以是绝对序号 `17` ，也可以是 `2^32 + 17` ，等等。。

checkpoint 用来解决这种歧义，它是最近一次被反包装的绝对序号，在 TCP 实现中，最后一个重组的字节序被当成这个 checkpoint。
