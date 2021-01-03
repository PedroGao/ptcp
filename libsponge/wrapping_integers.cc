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
    // n 是 absolute seqno，而 isn 是初始 seqno
    // 如果二者都是 64 位，则直接 isn + n 即可
    uint32_t offset = static_cast<uint32_t>(n & 0x00000000ffffffff);
    // 直接相加，如果 32 位溢出，直接再次从 0 开始
    return WrappingInt32{offset + isn.raw_value()};
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
    // n 是 seqno，返回 absolute seqno
    // checkpoint 是最近的一个 absolute seqno
    // checkpoint 的前面 32 位需要保留，后面的留给 offset
    // 计算 n 和 isn 之间的 offset
    // 1. n 大于等于 isn，则 offset = n - isn
    // 2. n 小于 isn，则 offset = 2^32 - 1 - isn + n;
    // 实际操作就是把算出来的 绝对序号分别加减1ul << 32做比较，选择与checkpoing差的绝对值最小的那个。
    uint32_t offset = n.raw_value() - isn.raw_value();
    uint64_t t = (checkpoint & 0xFFFFFFFF00000000) + offset;
    uint64_t ret = t;
    if (abs(int64_t(t + SEQ_EDGE - checkpoint)) < abs(int64_t(t - checkpoint)))
        ret = t + SEQ_EDGE;
    if (t >= SEQ_EDGE && abs(int64_t(t - SEQ_EDGE - checkpoint)) < abs(int64_t(ret - checkpoint)))
        ret = t - SEQ_EDGE;
    return ret;
}
