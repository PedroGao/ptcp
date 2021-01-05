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
    _time_since_last_segment_received = 0;  // 刷新接收时间
    auto header = seg.header();
    auto payload = seg.payload();
    // 如果处于 syn_sent 状态
    if (state() == TCPState::State::SYN_SENT) {
        if (header.ack && header.ackno != _sender.next_seqno()) {
            return;  // 如果是 ack，但是 ackno 不等于下一个 seqno，返回
        }
        if (header.rst && header.ack) {
            do_reset(false);  // 如果 rst 且 ack，则 reset，但不发包
            return;
        }
        if (!header.syn) {
            return;  // 没有 syn 直接返回
        }
    }
    // 处于监听状态
    if (state() == TCPState::State::LISTEN) {
        if (header.ack) {  // 监听收到 ack 包
            return;        // 直接返回，必须先收到 syn 包
        }
        if (!header.syn) {  // 监听状态下，不是 syn 包直接返回
            return;
        }
    }
    if (header.rst && header.seqno != _receiver.ackno()) {
        return;  // 如果 rst 且 seqno 不等于当前 ackno 则直接返回，因为不能 rst，还有包没有收到
    }
    if (header.rst) {
        do_reset();  // 重置且发包
        return;
    }
    bool ack_success = false, recv_success = false;
    bool seqno_is_correct = header.seqno == _receiver.ackno();  // seq 是累计确认的，所以 seqno == ackno
    if (header.ack) {                                           // 如果是 ack 包，则 ack_received
        ack_success = _sender.ack_received(header.ackno, header.win);
    }
    recv_success = _receiver.segment_received(seg);
    // 如果接收者没有数据了，但是发送方有数据
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()) {
        _end_before_fin = true;  // 在 fin 之前需要 end
    }
    // 如果均成功，但包没有数据，则返回
    if (ack_success && recv_success && seg.length_in_sequence_space() == 0 && seqno_is_correct) {
        return;
    }
    _sender.fill_window();  // 填充窗口
    // 没有包要发送，所以填充一个空包，用于窗口交流
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    // 发包
    send_out();
}

// 是否激活状态，前提条件见文档
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
    _sender.fill_window();  // 写数据后填充窗口
    send_out();             // 再发数据
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
        do_reset();  // 重置，见文档
        return;
    }
    if (state() == TCPState::State::ESTABLISHED) {
        _sender.fill_window();  // 已建立连接，则填充窗口
    }
    if (state() == TCPState::State::TIME_WAIT) {  // 超时
        if ((!_receiver.stream_out().input_ended()) &&
            (_linger_after_streams_finish == false || time_since_last_segment_received() >= 10 * _cfg.rt_timeout)) {
            _receiver.stream_out().input_ended();
            _linger_after_streams_finish = false;
        }
    }
    send_out();  // 发包
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();  // 关闭
    _sender.fill_window();            // 填充窗口
    send_out();                       // 发包
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
            do_reset();  // rst
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
        ask_receiver(seg);  // 设置包的 ack 和 window_size
        segments_out().push(seg);
    }
}

void TCPConnection::do_reset(bool send) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _sender.send_empty_segment();  // 将包给 _sender 的队列
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    seg.header().rst = true;
    if (send) {  // 如果发送，则将这个包发送到队列
        segments_out().push(seg);
    }
}