#include "tcp_segment.hh"

#include "parser.hh"
#include "util.hh"

#include <variant>

using namespace std;

//! \param[in] buffer string/Buffer to be parsed
//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
ParseResult TCPSegment::parse(const Buffer buffer, const uint32_t datagram_layer_checksum) {
    InternetChecksum check(datagram_layer_checksum);  // TCP 数据校验
    check.add(buffer);
    if (check.value()) {
        return ParseResult::BadChecksum;
    }

    NetParser p{buffer};
    _header.parse(p);
    _payload = p.buffer();
    return p.get_error();
}

size_t TCPSegment::length_in_sequence_space() const {
    // 当前 TCP 包在流中的大小，syn 和 fin 都算一个字节
    return payload().str().size() + (header().syn ? 1 : 0) + (header().fin ? 1 : 0);
}

//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
BufferList TCPSegment::serialize(const uint32_t datagram_layer_checksum) const {
    TCPHeader header_out = _header;
    header_out.cksum = 0;
    // 计算 payload 的校验和
    // calculate checksum -- taken over entire segment
    InternetChecksum check(datagram_layer_checksum);
    check.add(header_out.serialize());
    check.add(_payload);
    // 将校验字段写入头部
    header_out.cksum = check.value();

    BufferList ret;
    ret.append(header_out.serialize());
    ret.append(_payload);

    return ret;
}
