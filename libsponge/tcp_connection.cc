#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

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
    // 收到 tcp 包
    if (_active) {
        return;  // 非 active 直接返回
    }
    _time_since_last_segment_received = 0;  // 刷新接收时间
    if (in_syn_sent() && seg.header().ack && seg.payload().size() > 0) {
        // 处于 syn_sent 状态，不接受任何包
        return;
    }
    _receiver.segment_received(seg);
    // 收到 syn 包，且目前还没有发送任何数据，所以需要向对方发送 ack 包和 syn 包
    if (seg.header().syn && _sender.next_seqno_absolute() == 0) {
        connect();
        return;
    }
    if (seg.header().rst) {
        if (in_syn_sent() && !seg.header().ack) {
            // 如果在 syn_sent 状态中，但没有 ack，则直接返回
            return;
        }
        // 设置 sender 和 receiver error
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // 设置 active 为 false
        _active = false;
    }
    send_segment();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    // 写输入
    size_t sz = _sender.stream_in().write(data);
    send_segment();  // 发包
    return sz;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!_active) {
        return;  // 直接退出
    }
    // 计时
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    // 检查重试次数
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // 发送 rst 包
        send_rst_segment();
    }
    // 因为 _sender tick 以后就会有新的包加入队列，所以再次发包
    send_segment();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // 发送最后的包
    send_segment();
}

void TCPConnection::connect() {
    // 连接的时候发送 syn 包
    send_segment();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // 发送一个 rst 包给对面
            // Your code here: need to send a RST segment to the peer
            send_rst_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::send_segment() {
    // outgoing segments: (seqno, syn , payload, and fin )
    // 在 send segment 之前，需要向 receiver 拿 ackno 和 window_size
    // 如果有 ackno，还需要设置 ack 标志
    _sender.fill_window();
    // fill_window 后，Segment 在 _sender 的队列中
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();  // 弹出
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().seqno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        // 需要发送 rst
        if (_sent_rst) {
            seg.header().rst = true;
            _sent_rst = false;
        }
        // 将包从 sender 从移除然后设置头部数据后加入到 connection 中
        _segments_out.push(seg);
    }
}

void TCPConnection::send_rst_segment() {
    // 设置 sender 和 receiver error
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    // 设置 active 为 false
    _active = false;
    _sent_rst = true;  // 设置 rst，后面重发的包也需要设置
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();  // 因为要发送 rst，如果 sender 没有任何数据，则必须发送一个空包给队列
    }
    send_segment();
}

void TCPConnection::reset_linger() {
    // If the inbound stream ends before the TCPConnection
    // has reached EOF on its outbound stream, this variable needs to be set to false.
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    // 关闭的条件见文档
    if (_receiver.stream_out().input_ended() && _receiver.unassembled_bytes() == 0 && _sender.stream_in().eof() &&
        _sender.bytes_in_flight() == 0) {
        if (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
}

bool TCPConnection::in_listen() { return !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0; }

bool TCPConnection::in_syn_recv() { return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); }

bool TCPConnection::in_syn_sent() {
    return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
}