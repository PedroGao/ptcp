#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 接收 TCP 报文
bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    bool ret{};              // 当 SYN 或者 FIN 包的时候设位 true
    size_t abs_seqno = 0;    // 绝对序号
    size_t length;           // 报文数据长度，不包含 SYN 和 FIN，因此需要 SYN 和 FIN 包要 -1
    if (seg.header().syn) {  // 如果 TCP 头部有 syn
        if (_syn) {          // 如果已经有 syn 了，则直接拒绝
            return false;
        }
        _syn = true;  // 设置 syn
        ret = true;
        _isn = seg.header().seqno.raw_value();  // 因为是 SYN，所以是第一个报文，因此需要设置 isn
        abs_seqno = 1;                          // 绝对序号位 1，SYN 消耗一个长度
        _base = 1;                              // _base 设为 1，暂时不理解
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
    else if (seg.length_in_sequence_space() == 0 && abs_seqno == _base) {  // 如果数据长度为 0，且绝对序号等于 base
        return true;                                                       // 直接返回 true
    } else if (abs_seqno >= _base + window_size() ||
               abs_seqno + length <= _base) {  // 绝对序号大于 base + 窗口大小，或者 绝对序号 + 数据长度 <= base
        if (!ret) {                            // 即超过了接收范围，或者已经被接受过了
            return false;                      // 如果不是特殊的 SYN，FIN 包，则直接返回 false
        }
    }
    // 将数据传给重组器
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    // _base 是下一个字节序号，因此要加一
    _base = _reassembler.head_index() + 1;
    if (_reassembler.input_ended()) {  // FIN  包也需要计数
        _base++;
    }
    return true;
}

// the sequence number of the first byte in the stream that the receiver hasn't received.
// 未收到数据的第一个字节序号
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_base > 0) {
        return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
    }
    // 如果 base 等于 0，即还没有受到
    return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
