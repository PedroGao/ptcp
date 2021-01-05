#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn && _syn) {
        // 在已经收到 syn 的情况下，再次收到 syn 包，直接丢弃包
        return false;
    }
    bool first_syn = false;
    if (seg.header().syn) {
        _syn = true;
        _abs_seqno = 1;  // syn 消耗一个序号，这个地方很重要，_abs_seqno 是计算后面 _abs_seqno 的重要根据
        _isn = seg.header().seqno;
        first_syn = true;
    }
    // 没有 syn 的包全部拒绝
    if (!_syn) {
        return false;
    }
    // 判断有没有多次 fin
    if (seg.header().fin && _fin) {
        return false;
    }
    // 只有在包没有任何问题的情况下，才能计算 abs_seqno，不然 abs_seqno 会被其它包污染
    // 当前包的 abs_seqno，如果是 fin 包，此处已经根据 seqno 计算而来
    if (!first_syn) {
        // 当前包的 abs seqno
        uint64_t cur = unwrap(seg.header().seqno, _isn, _abs_seqno);
        if (cur == 0) {  // 如果再已经 syn 的情况下还是 0 则直接返回
            return false;
        }
        _abs_seqno = cur;
    }
    // 结束包
    // 这里有一个终极大 BUG，如果 fin 包提前到达，那么比 fin 包序号小的包还未达到，但也应该被接收
    // 所以这里还需要判断
    if (seg.header().fin) {
        _fin = true;
        _abs_seqno += 1;  // fin 也消耗一个字节
    }
    // 加入数据，此处的 index 可以由 abs_seqno 计算而得
    // 因为 fin 包的存在，所以即使没有数据，也要 push
    // 注意：如果当前是 fin 包，那么需要减一，其它情况均不需要
    size_t index = _abs_seqno - (_syn ? 1 : 0) - (seg.header().fin ? 1 : 0);
    _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_abs_seqno == 0) {  // 绝对值为 0，则未收到任何包，此处处于 listen 状态
        return std::nullopt;
    }
    // 即使有 fin 包，那么不能当做 ackno 直接计算
    // 这里有一个终极大 BUG，如果 fin 包提前到达，那么比 fin 包序号小的包还未达到，但也应该被接收
    // 所以还需判断 _reassembler 是否 empty
    const size_t first_unassembled =
        _reassembler.head_index() + (_syn ? 1 : 0) + ((_fin && _reassembler.empty()) ? 1 : 0);
    return wrap(static_cast<uint64_t>(first_unassembled), _isn);
}

size_t TCPReceiver::window_size() const {
    // 这里的 window_size 应该是 容量 - 已经在 StreamByte 中的字节数
    // 即剩下可以接受且重组的字节数才是窗口大小
    return _capacity - _reassembler.stream_out().buffer_size();
}
