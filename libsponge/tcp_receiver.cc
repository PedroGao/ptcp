#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // auto header = seg.header();
    // auto payload = seg.payload();
    // if (header.syn && _syn) {
    //     // 在已经收到 syn 的情况下，再次收到 syn 包，直接丢弃包
    //     return false;
    // }
    // bool first_syn = false;
    // if (header.syn) {
    //     _syn = true;
    //     _abs_seqno = 1;  // syn 消耗一个序号，这个地方很重要，_abs_seqno 是计算后面 _abs_seqno 的重要根据
    //     _isn = header.seqno;
    //     first_syn = true;
    // }
    // // 没有 syn 的包全部拒绝
    // if (!_syn) {
    //     return false;
    // }
    // // 判断有没有多次 fin
    // if (header.fin && _fin) {
    //     return false;
    // }
    // // 只有在包没有任何问题的情况下，才能计算 abs_seqno，不然 abs_seqno 会被其它包污染
    // // 当前包的 abs_seqno，如果是 fin 包，此处已经根据 seqno 计算而来
    // if (!first_syn) {
    //     // 当前包的 abs seqno
    //     uint64_t cur = unwrap(header.seqno, _isn, _abs_seqno);
    //     if (cur == 0) {  // 如果再已经 syn 的情况下还是 0 则直接返回
    //         return false;
    //     }
    //     // 判断 cur 与 _abs_seqno 之间的大小，如果超过了 window_size 则拒绝
    //     if (_abs_seqno + window_size() <= cur) {
    //         return false;
    //     }
    //     _abs_seqno = cur;
    // }
    // // 结束包
    // // 这里有一个终极大 BUG，如果 fin 包提前到达，那么比 fin 包序号小的包还未达到，但也应该被接收
    // // 所以这里还需要判断
    // if (header.fin) {
    //     _fin = true;
    //     _abs_seqno += 1;  // fin 也消耗一个字节
    // }
    // // 加入数据，此处的 index 可以由 abs_seqno 计算而得
    // // 因为 fin 包的存在，所以即使没有数据，也要 push
    // // 注意：如果当前是 fin 包，那么需要减一，其它情况均不需要
    // size_t index = _abs_seqno - (_syn ? 1 : 0) - (header.fin ? 1 : 0);
    // _reassembler.push_substring(payload.copy(), index, header.fin);
    // return true;

    bool ret{};              // 当 SYN 或者 FIN 包的时候设位 true
    size_t abs_seqno = 0;    // 绝对序号
    size_t length;           // 报文数据长度，不包含 SYN 和 FIN，因此需要 SYN 和 FIN 包要 -1
    if (seg.header().syn) {  // 如果 TCP 头部有 syn
        if (_syn) {          // 如果已经有 syn 了，则直接拒绝
            return false;
        }
        _syn = true;  // 设置 syn
        ret = true;
        _isn = seg.header().seqno;  // 因为是 SYN，所以是第一个报文，因此需要设置 isn
        abs_seqno = 1;              // 绝对序号位 1，SYN 消耗一个长度
        _abs_seqno = 1;             // _base 设为 1，暂时不理解
        length = seg.length_in_sequence_space() - 1;  // 数据长度 - 1，因此第一个握手可能直接携带数据
        if (length == 0) {                            // 如果整个 TCP 包只有一个 SYN
            return true;                              // 返回
        }
    } else if (!_syn) {
        // 如果没有之前没有 SYN，未同步，则拒绝当前的包，自动丢弃
        return false;
    } else {  // 之前已经同步，但现在没有同步
              // 计算绝对序号，传入 seqno，isn 和最近一次 abs_seqno
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
        length = seg.length_in_sequence_space();  // 数据长度
    }
    if (seg.header().fin) {
        if (_fin) {  // 如果已经得到了 FIN 包，则拒绝后面的所有包
            return false;
        }
        _fin = true;
        ret = true;
    }
    // 既没有 SYN 也没有 FIN
    else if (seg.length_in_sequence_space() == 0 && abs_seqno == _abs_seqno) {  // 如果数据长度为 0，且绝对序号等于 base
        return true;                                                            // 直接返回 true
    } else if (abs_seqno >= _abs_seqno + window_size() ||
               abs_seqno + length <= _abs_seqno) {  // 绝对序号大于 base + 窗口大小，或者 绝对序号 + 数据长度 <= base
        if (!ret) {                                 // 即超过了接收范围，或者已经被接受过了
            return false;                           // 如果不是特殊的 SYN，FIN 包，则直接返回 false
        }
    }
    // 将数据传给重组器
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    // _base 是下一个字节序号，因此要加一
    _abs_seqno = _reassembler.head_index() + 1;
    if (_reassembler.stream_out().input_ended()) {  // FIN  包也需要计数
        _abs_seqno++;
    }
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
