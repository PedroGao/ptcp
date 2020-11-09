#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);
    // WrappingInt32 表示的是在 TCP 中传输的 seqno，n 是 64 位的绝对字节流序列，即 absolute seqno
    // 因此此处的 wrap 的意思时得到 n 个字节后的传输序列
    // 因此 seqno = absolute seqno + isn(初始化号)
    return WrappingInt32(static_cast<uint32_t>(n) + isn.raw_value());
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // DUMMY_CODE(n, isn, checkpoint);
    // 将 seqno 转换为 absolute seqno，并且这个 abs seqno 是最靠近 checkpoint 的
    // 得到偏移量
    uint32_t offset = n.raw_value() - isn.raw_value();
    // checkpoint 表示最近一次转化得到的 absolute seqno
    // 通过掩码来得到前面 32 位的数据，然后加上偏移量，得到当前的 absolute seqno
    uint64_t t = (checkpoint & 0xFFFFFFFF00000000) + offset;
    // 得到 absolute seqno 后
    uint64_t ret = t;
    // 判断 t 与 checkpoint 之间的大小关系
    // 如果 t + 1ul << 32 - checkpoint 的绝对值 < t - checkpoint 的绝对值
    // 说实话，这里我真的没看懂，后面再说把，先不纠结这些细节
    if (abs(int64_t(t + (1ul << 32) - checkpoint)) < abs(int64_t(t - checkpoint)))
        ret = t + (1ul << 32);
    // 如果 t 已经大于 1ul << 32 且
    if (t >= (1ul << 32) && abs(int64_t(t - (1ul << 32) - checkpoint)) < abs(int64_t(ret - checkpoint)))
        ret = t - (1ul << 32);
    return ret;
}
