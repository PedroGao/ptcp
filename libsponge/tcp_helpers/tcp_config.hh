#ifndef SPONGE_LIBSPONGE_TCP_CONFIG_HH
#define SPONGE_LIBSPONGE_TCP_CONFIG_HH

#include "address.hh"
#include "wrapping_integers.hh"

#include <cstddef>
#include <cstdint>
#include <optional>

//! Config for TCP sender and receiver
// TCP 配置，终于可以输入中文了～
class TCPConfig {
  public:
    static constexpr size_t DEFAULT_CAPACITY = 64000;  //!< Default capacity 默认容量大小
    static constexpr size_t MAX_PAYLOAD_SIZE =
        1452;  //!< Max TCP payload that fits in either IPv4 or UDP datagram 最大 TCP 包大小
    static constexpr uint16_t TIMEOUT_DFLT = 1000;  //!< Default re-transmit timeout is 1 second 默认重试的时间
    static constexpr unsigned MAX_RETX_ATTEMPTS = 8;  //!< Maximum re-transmit attempts before giving up 最大重试此书

    uint16_t rt_timeout = TIMEOUT_DFLT;  //!< Initial value of the retransmission timeout, in milliseconds 重试时间
    size_t recv_capacity = DEFAULT_CAPACITY;  //!< Receive capacity, in bytes 接收窗口大小
    size_t send_capacity = DEFAULT_CAPACITY;  //!< Sender capacity, in bytes 发送窗口容量
    std::optional<WrappingInt32> fixed_isn{};
};

#endif  // SPONGE_LIBSPONGE_TCP_CONFIG_HH
