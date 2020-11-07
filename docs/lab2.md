## lab2

## 注意点

* 在 TCP 中， SYN(开始) 和 FIN(结束) 同样消耗序号。
* TCP 序号不是从零开始。

|  element   | SYN  | c|a  |t |FIN|
|  ----  | ----  |--- | ---|---  | ---|
| seqno  | 2^32 - 2 | 2^32 -1  | 0 | 1 | 2|
| absolute seqno  | 0 | 1| 2|3|4|
| stream index  |  | 0 | 1| 2| | 
