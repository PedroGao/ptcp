#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    bool ret{};
    size_t abs_seqno = 0;
    size_t length;
    if (seg.header().syn) {
        if (_syn) {  // 如果已经有 syn 了，则直接拒绝
            return false;
        }
        _syn = true;
        ret = true;
        _isn = seg.header().seqno.raw_value();
        abs_seqno = 1;
        _base = 1;
        length = seg.length_in_sequence_space() - 1;
        if (length == 0) {  // 如果整个 TCP 包只有一个 SYN
            return true;
        }
    } else if (!_syn) {
        // 如果没有之前没有 SYN，未同步，则拒绝当前的包，自动丢弃
        return false;
    } else {
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
        length = seg.length_in_sequence_space();
    }
    if (seg.header().fin) {
        if (_fin) {  // 如果已经得到了 FIN 包，则拒绝后面的所以包
            return false;
        }
        _fin = true;
        ret = true;
    }
    // 只有 SYN 没有 FIN
    else if (seg.length_in_sequence_space() == 0 && abs_seqno == _base) {
        return true;
    } else if (abs_seqno >= _base + window_size() || abs_seqno + length <= _base) {
        if (!ret) {
            return false;
        }
    }
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    _base = _reassembler.head_index() + 1;
    if (_reassembler.input_ended()) {  // FIN  包也需要计数
        _base++;
    }
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_base > 0) {
        return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
    }
    return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
