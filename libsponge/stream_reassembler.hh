#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <set>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    struct node {
        size_t begin = 0;
        size_t length = 0;
        std::string data{};
        //! 通过节点的开始节点来排序节点
        bool operator<(const node &t) const { return begin < t.begin; }
    };

    std::set<node> _nodes{};        //! 已接受但未重组的 substring
    size_t _unassembled_bytes = 0;  //! 未重组的字节数
    size_t _head_index = 0;         //! 头部节点
    bool _eof{};                    //! 输入完毕

    long merge(node &node1, const node &node2);  //! 合并两个 node，合并的结果为 node1，return 合并的字节数

    void end_input();  //! output eof

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    //! \brief 已经重组的字节序号
    //! \returns 已经重组的字节序号
    size_t head_index() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
