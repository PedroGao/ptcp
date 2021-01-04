# Lab 4 Writeup

## Hints

- 本实验实现的 `tcp` 既可以用工作在`udp`上，也可以工作在`ip`上。
- tcp 如果 `time_since_last_segment_received() >= 10 * _cfg.rt_timeout` 触发，即最后一个包接受后过去了太久，此时需要关闭
  tcp 连接。
