#include "tcp_state.hh"

using namespace std;

// 得到当前 TCP 接受者的状态，注意 TCP 的多个状态由
// 接受者和发送者单独维护
string TCPState::state_summary(const TCPReceiver &receiver) {
    if (receiver.stream_out().error()) {
        return TCPReceiverStateSummary::ERROR;
    } else if (not receiver.ackno().has_value()) {
        return TCPReceiverStateSummary::LISTEN;  // 如果没有 ack no，表示正在监听
    } else if (receiver.stream_out().input_ended()) {
        return TCPReceiverStateSummary::FIN_RECV;  // 如果输出完毕，表示已经接受到了 FIN
    } else {
        return TCPReceiverStateSummary::SYN_RECV;  // 获得已经收到了 SYN
    }
}
