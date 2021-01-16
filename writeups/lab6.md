# Lab 6 Writeup

实现 IP 路由协议，遵循最长匹配原则。

只有当 next_hop 为空的时候代表该路由器为目的路由器，此时 next_hop 取 dgram 的目的 IP 地址
