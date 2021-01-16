#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check_lab4`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    // sender remaining_capacity
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    // sender 中正在飞行的字节数
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    // receiver 中未重组的字节数
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    // 距离收到上一个 tcp 包过去了多久
    return _time_since_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!_active)  // 如果当前处于未激活状态，则直接返回
        return;
    _time_since_last_segment_received = 0;
    // 如果已经发送了 SYN 包，但是接收的包仍有 SYN，则返回
    if (state() == TCPState::State::SYN_SENT && seg.header().ack && seg.payload().size() > 0) {
        return;
    }
    bool send_empty = false;
    if (_sender.next_seqno_absolute() > 0 && seg.header().ack) {
        if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
            send_empty = true;
        }
    }
    // 判断报文是否被接收了
    bool recv_flag = _receiver.segment_received(seg);
    if (!recv_flag) {
        send_empty = true;
    }
    if (seg.header().syn && _sender.next_seqno_absolute() == 0) {
        connect();
        return;
    }
    if (seg.header().rst) {
        // reset 包
        if (state() == TCPState::State::SYN_SENT && !seg.header().ack) {
            return;
        }
        unclean_shutdown(false);
        return;
    }
    if (seg.length_in_sequence_space() > 0) {
        send_empty = true;
    }
    // 是否发送空报文
    if (send_empty) {
        if (_receiver.ackno().has_value() && _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }
    push_segments_out();
}

// 是否激活状态，前提条件见文档
bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    // 写输入
    size_t ret = _sender.stream_in().write(data);
    push_segments_out();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!_active) {
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        unclean_shutdown(true);
    }
    push_segments_out();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    push_segments_out();  // 发包
}

void TCPConnection::connect() {
    // 建立连接的时候，发送一个 SYN 报文
    push_segments_out(true);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // 发送一个 rst 包给对面
            // Your code here: need to send a RST segment to the peer
            unclean_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

bool TCPConnection::push_segments_out(bool send_syn) {
    // 在没有受到 SYN 之前先不发送 SYN
    _sender.fill_window(send_syn || state() == TCPState::State::SYN_SENT);
    TCPSegment seg;
    while (!_sender.segments_out().empty()) {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if (_need_send_rst) {
            _need_send_rst = false;
            seg.header().rst = true;
        }
        _segments_out.push(seg);
    }
    clean_shutdown();
    return true;
}

void TCPConnection::unclean_shutdown(bool send_rst) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;
    if (send_rst) {
        _need_send_rst = true;
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        push_segments_out();
    }
}

bool TCPConnection::clean_shutdown() {
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
    return !_active;
}