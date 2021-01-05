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
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    // 已发送但未被确认的字节数
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    // 读取数据装包，并发包
    // 首先要发送一个 SYN 包
    if (!_syn) {
        _syn = true;
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        return;  // 发送 syn 包后等待 ack 才继续发包
    }
    if (!_syn) {
        return;  // 其它情况下，未 syn 则不进行任何发送操作
    }
    if (_fin) {
        return;  // 都已经 fin 了，还发个鬼啊
    }
    // _window_size 是 0 的时候，sender 将 _window_size 当作 1 看待
    size_t win = _window_size > 0 ? _window_size : 1;
    size_t remain;
    // 当窗口没有满，且无 fin
    // 这个地方 win 是接收者建议的窗口大小
    // 但是发送的数据不能直接以这个窗口作为发送窗口，因为还有一些数据处于 flight 的状态
    // 这些数据本身还需要被确认，因此发送窗口必须考虑到这些数据的存在，所以需要减掉
    while ((remain = (win - (_next_seqno - _recv_ackno))) > 0 && !_fin) {
        size_t sz = min(remain, TCPConfig::MAX_PAYLOAD_SIZE);
        TCPSegment seg;
        string payload = _stream.read(sz);
        seg.payload() = Buffer(std::move(payload));
        // 如果包的大小小于窗口大小，且已经 eof 了，那么作为 fin 包发送
        if (seg.length_in_sequence_space() < win && _stream.eof()) {
            seg.header().fin = true;
            _fin = true;
        }
        // 注意：fin 包可能没有任何数组，但是如果提前判读包的大小，那么 fin 包
        // 在被设置 header 之前就被 return 了，当前的 fin 包就丢失了
        if (seg.length_in_sequence_space() == 0) {
            return;  // 空包直接返回，注意
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 算出包的 abs_ackno
    uint64_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    // 这个包无法确认，累积确认，必须按照顺序一步一步来
    if (abs_ackno > _next_seqno) {
        return false;  // 无需确认
    }
    // 最新的 window_size 得确认
    _window_size = window_size;
    if (abs_ackno <= _recv_ackno) {  // 注意，如果是等于，那么其实也已经确认过了
        return true;                 // 已经确认过，无法再次确认
    }
    _recv_ackno = abs_ackno;  // 更新 recv_ack
    // 更新确认包以后，删除 _outstanding_segments 中的确认包
    while (!_outstanding_segments.empty()) {
        TCPSegment front = _outstanding_segments.front();
        // 这个地方使用 _next_seqno，因为 seqno 与 _next_seqno 关联最深，seqno 由 _next_seqno 计算而得
        uint64_t front_abs_seqno = unwrap(front.header().seqno, _isn, _next_seqno);
        if (front_abs_seqno + front.length_in_sequence_space() <= _recv_ackno) {
            // 头部 seqno < _recv_ackno，这个包被确认了，直接 pop
            _bytes_in_flight -= front.length_in_sequence_space();
            _outstanding_segments.pop();
        } else {
            break;  // 开头的没有，那么后面的肯定也没有了吗，直接 break
        }
    }
    // 这个地方为什么需要在接受包以后立马又发送包呢？
    // 必须得，因为这就是交互逻辑，单元测试里面大量测试没有直接 fill_window，而是调用 ack_received
    // fill_window();  // 这个地方不需要 fill_window，sender 将 fill_window 的决定权交给了 connection
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    if (!_outstanding_segments.empty()) {
        _timer = true;
        _time_tick = 0;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 过去了 ms_since_last_tick 毫秒
    // 这个时候需要检查是否超时，对于超时的包要重传
    if (!_timer) {
        return;  // 如果所有的包被确认完毕，无需重发，则无需定时器，直接返回即可
    }
    _time_tick += ms_since_last_tick;  // 计时
    // 超时，重新发送包
    if (_time_tick >= _retransmission_timeout && !_outstanding_segments.empty()) {
        // 发送 _outstanding_segments 的队顶包，注意每次只重发第一个包
        _segments_out.push(_outstanding_segments.front());
        _consecutive_retransmissions++;  // 重传次数 +1
        // If the window size is nonzero，Double the value of RTO
        // 注意：如果 _window_size 不为 0，则超时加倍，否则，不加倍
        if (_window_size != 0) {           // 不等于 0 的情况下才 *2
            _retransmission_timeout *= 2;  // 超时时间 *2
        }
        _timer = true;
        _time_tick = 0;  // 重新计时
    }
    if (_outstanding_segments.empty()) {
        _timer = false;  // 如果 _outstanding_segments 为空，则直接关闭 timer
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    // 发送空包
    // 注意：空包不需要被存储在 _outstanding_segments 中，也无需被重传
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    // 这里最有意思的是，发送包起始就是向 _segments_out 队列中添加包
    // 至于什么时候真正的发送由后面的流程和网卡操作
    _segments_out.push(seg);
}

// 发送包
void TCPSender::send_segment(TCPSegment &segment) {
    segment.header().seqno = wrap(_next_seqno, _isn);
    const size_t seg_space = segment.length_in_sequence_space();
    _next_seqno += seg_space;       // 更新 _next_seqno
    _bytes_in_flight += seg_space;  // 更新 _bytes_in_flight
    _segments_out.push(segment);
    _outstanding_segments.push(segment);
    if (!_timer) {  // 如果计时没有开始，则开始计时
        _timer = true;
        _time_tick = 0;
    }
}