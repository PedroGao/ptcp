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
    return _cfg.send_capacity - _sender.bytes_in_flight();
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
    _time_since_last_segment_received = 0;  // 刷新接收时间
    auto header = seg.header();
    auto payload = seg.payload();
    // 如果处于 syn_sent 状态
    if (state() == TCPState::State::SYN_SENT) {
        if (header.ack && header.ackno != _sender.next_seqno()) {
            return;
        }
        if (header.rst && header.ack) {
            do_reset(false);
            return;
        }
        if (!header.syn) {
            return;
        }
    }
    if (state() == TCPState::State::LISTEN) {
        if (header.ack) {
            return;
        }
        if (!header.syn) {
            return;
        }
    }
    if (header.rst && header.seqno != _receiver.ackno()) {
        return;
    }
    if (header.rst) {
        do_reset();
        return;
    }
    bool ack_success = false, recv_success = false;
    bool seqno_is_correct = header.seqno == _receiver.ackno();
    if (header.ack) {
        ack_success = _sender.ack_received(header.ackno, header.win);
    }
    recv_success = _receiver.segment_received(seg);
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()) {
        _end_before_fin = true;
    }
    if (ack_success && recv_success && seg.length_in_sequence_space() == 0 && seqno_is_correct) {
        return;
    }
    _sender.fill_window();
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    send_out();
}

bool TCPConnection::active() const {
    // unclean shutdown
    if (_sender.stream_in().error() && _receiver.stream_out().error()) {
        return false;
    }
    // clean shutdown
    if ((_sender.stream_in().eof() && _sender.bytes_in_flight() == 0) && (_receiver.stream_out().input_ended()) &&
        (_linger_after_streams_finish == false || time_since_last_segment_received() >= 10 * _cfg.rt_timeout)) {
        return false;
    }
    return true;
}

size_t TCPConnection::write(const string &data) {
    // 写输入
    auto size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_out();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    if (_end_before_fin) {
        _linger_after_streams_finish = false;
    }
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        do_reset();
        return;
    }
    if (state() == TCPState::State::ESTABLISHED) {
        _sender.fill_window();
    }
    if (state() == TCPState::State::TIME_WAIT) {
        if ((!_receiver.stream_out().input_ended()) &&
            (_linger_after_streams_finish == false || time_since_last_segment_received() >= 10 * _cfg.rt_timeout)) {
            _receiver.stream_out().input_ended();
            _linger_after_streams_finish = false;
        }
    }
    send_out();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_out();
}

void TCPConnection::connect() {
    // 连接的时候发送 syn 包
    _sender.fill_window();
    send_out();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // 发送一个 rst 包给对面
            // Your code here: need to send a RST segment to the peer
            do_reset();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::ask_receiver(TCPSegment &ongoing_seg) const {
    auto ackno = _receiver.ackno();
    if (ackno.has_value()) {
        ongoing_seg.header().ack = true;
        ongoing_seg.header().ackno = ackno.value();
        ongoing_seg.header().win = _receiver.window_size();
    }
}

void TCPConnection::send_out() {
    while (!_sender.segments_out().empty()) {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        ask_receiver(seg);
        segments_out().push(seg);
    }
}

void TCPConnection::do_reset(bool send) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _sender.send_empty_segment();
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    seg.header().rst = true;
    if (send) {
        segments_out().push(seg);
    }
}