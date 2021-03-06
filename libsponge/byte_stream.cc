#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// Hints：
// 1. The byte stream is finite: the writer can end the input, and then no more bytes can be written.
// 2. As the reader reads bytes and drains them from the stream, the writer is allowed to write more.
// 3. capacity 只会限制 ByteStrem 在内存的容量，不会限制读写数据

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    // 注意：这里是多少可写，所以是均衡后的数据
    size_t len = data.size();
    size_t will_write = std::min(len, remaining_capacity());
    string s = string().assign(data.begin(), data.begin() + will_write);
    _buffer.append(BufferList(move(s)));
    _bytes_write += will_write;
    return will_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = std::min(len, _buffer.size());
    const string s = _buffer.concatenate();
    return string().assign(s.begin(), s.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // 注意：这里是有多少字节可读，所有就是 buffer 的容量
    size_t length = std::min(len, _buffer.size());
    _buffer.remove_prefix(length);
    _bytes_read += length;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _bytes_write; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return max<size_t>(0, _capacity - _bytes_write + _bytes_read); }
