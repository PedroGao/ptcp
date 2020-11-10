#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

// 默认第一个包是 SYN，所以 send_syn 默认 true
void TCPSender::fill_window(bool send_syn) {
    // 首先发送一个 SYN 包
    if (!_syn && send_syn) {  // 如果没有同步过，且 send_syn 为 true
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        _syn = true;
        return;
    }
    // 如果 window_size 是 0，则设为 1
    size_t win = _window_size > 0 ? _window_size : 1;
    size_t remain;  // 滑动窗口中还剩多少
    // 如果没有发送 FIN 包，且滑动窗口还没满，则一直发
    // 考虑到如果 win < _next_seqno - _recv_ackno，那么 remain 是负数，会产生 bug，所以改一下条件判断
    while ((remain = win - (_next_seqno - _recv_ackno)) > 0 && !_fin) {
        // 得到包的大小，如果剩下的窗口太大，则使用默认窗口大小
        size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, remain);
        TCPSegment seg;
        string str = _stream.read(size);
        seg.payload() = Buffer(std::move(str));
        // 如果当前在数据大小小于窗口大小，且已经传输完毕 添加 FIN 包
        // 这个地方一定要先判断报文的数据大小 要小于 窗口大小
        // 如果报文的大小大于窗口大小，则一次无法发完，所以无法直接发送 fin 包
        if (seg.length_in_sequence_space() < win && _stream.eof()) {
            seg.header().fin = true;
            _fin = true;
        }
        // 如果数据为空，则直接返回
        // 这个地方一定要判断，不然会引起死循环，因为窗口默认的大小是 1，因此一定会产生一个报文，但是如果报文为空
        // 则表示该报文没有任何数据，没有 SYN，没有 FIN 包
        if (seg.length_in_sequence_space() == 0) {
            return;
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
// 接受一个新的 ack_no
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    // 发送方收到一个ackno代表了接收方已经收到ackno之前的所有段。
    // 得到绝对的 ask 序号
    size_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    // 如果超过了 _next_seqno，直接返回
    if (abs_ackno > _next_seqno) {
        return false;
    }
    // 如果 ackno 是合法的，修改滑动窗口的大小
    _window_size = window_size;
    // 如果已经被 ack 过了
    if (abs_ackno <= _recv_ackno) {
        return true;
    }
    _recv_ackno = abs_ackno;  // 更新最近一次 recv_ackno
    // 移除在 ackno 之前的包
    while (!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        // 如果当前包的绝对序号 + 包的长度 仍然小于已经确认的 ackno，则丢弃这个包，因为已经被确认了
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
        } else {
            break;
        }
    }
    // 重新发送
    fill_window();
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmission = 0;
    // 如果仍有 flight 中的包，则开启定时器
    if (!_segments_outstanding.empty()) {
        _timer_running = true;
        _timer = 0;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 提醒 sender 已经过去了多久
    // DUMMY_CODE(ms_since_last_tick);
    _timer += ms_since_last_tick;  // 增加计时器
    // 如果已经超时，且仍有在 flight 中的包
    if (_timer >= _retransmission_timeout && !_segments_outstanding.empty()) {
        // 将 flight 中的包 push 到要发送的队列中
        _segments_out.push(_segments_outstanding.front());
        _consecutive_retransmission++;  // 重试次数 ++
        _retransmission_timeout *= 2;   // 重试时间 *2
        _timer_running = true;          // 定时器开始
        _timer = 0;                     // 重新计时
    }
    // 如果 flight 中的包为空，则关闭计时器
    if (_segments_outstanding.empty()) {
        _timer_running = false;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment() {
    // 发送空的 TCP 报文，空的报文不用进入 flight 队列
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    // 发送报文
    seg.header().seqno = wrap(_next_seqno, _isn);
    // 下一个序号
    _next_seqno += seg.length_in_sequence_space();
    // 飞行中的字节数
    _bytes_in_flight += seg.length_in_sequence_space();
    _segments_outstanding.push(seg);  // 包发送了，但未被确认
    _segments_out.push(seg);          // 包已经发送
    // 如果定时器没有启动，则启动定时器
    if (!_timer_running) {
        _timer_running = true;
        _timer = 0;
    }
}
