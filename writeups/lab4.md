# Lab 4 Writeup

## Hints

- 本实验实现的 `tcp` 既可以用工作在`udp`上，也可以工作在`ip`上。
- tcp 如果 `time_since_last_segment_received() >= 10 * _cfg.rt_timeout` 触发，即最后一个包接受后过去了太久，此时需要关闭
  tcp 连接。

## 重点

- FSM 状态机之间的转换，所以`state`这个函数很重要~

## 参考资料

- tcp 状态详解：https://blog.csdn.net/wuji0447/article/details/78356875。
- faqs：https://cs144.github.io/lab_faq.html。
- https://blog.csdn.net/weixin_44179892/article/details/110675514
