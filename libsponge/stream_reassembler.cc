#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 重组完毕的 substring 再输入到 _output 中
    // 判断是否超过的 capacity
    // size_t sz = data.size();
    // if (eof) {
    //     _eof = eof;  // 不管怎么样，先搞定 eof
    // }
    // // 当前字符串的末尾序号已经被重组了，则直接返回
    // if (sz == 0 || index + sz < _head_index) {
    //     end_input();
    //     return;
    // }
    // node cur;
    // if (index < _head_index) {
    //     // 截取有用的后半部分
    //     size_t offset = _head_index - index;
    //     cur.begin = _head_index;
    //     cur.length = sz - offset;
    //     cur.data = data.substr(offset);
    // } else {
    //     cur.begin = index;
    //     cur.data = data;
    //     cur.length = sz;
    // }
    // _unassembled_bytes += cur.length;
    // // 合并字节
    // // 二分查找，返回第一个大于等于 cur_node.begin 的节点，如果没找到，返回末尾的迭代器位置
    // // 向后合并，cur.begin = 90  next.begin = 100
    // long merged_bytes = 0;
    // while (true) {
    //     auto next = _nodes.lower_bound(cur);
    //     if (next == _nodes.end()) {
    //         break;
    //     }
    //     // 合并 cur 和 next
    //     long m = merge(cur, *next);
    //     if (m < 0) {
    //         // 无合并
    //         break;
    //     }
    //     // 有合并，删除已经合并的 next 节点
    //     _nodes.erase(next);
    //     merged_bytes += m;
    // }
    // auto iter = _nodes.lower_bound(cur);
    // // 向前合并
    // // 如果已经是第一个节点了，同时也是当前节点，则已经不用向前合并了
    // while (iter != _nodes.begin()) {
    //     // 否则 iter--，得到前面一个节点进行合并
    //     iter--;
    //     long m = merge(cur, *iter);
    //     if (m < 0) {
    //         // 无合并
    //         break;
    //     }
    //     // 有合并，删除已经合并的 next 节点
    //     _nodes.erase(iter);
    //     merged_bytes += m;
    //     iter = _nodes.lower_bound(cur);
    // }
    // _unassembled_bytes -= merged_bytes;
    // // 插入 cur
    // _nodes.insert(cur);
    // while (!_nodes.empty() && _nodes.begin()->begin == _head_index) {
    //     auto begin = _nodes.begin();
    //     size_t written = _output.write(begin->data);
    //     _head_index += written;
    //     _unassembled_bytes -= written;
    //     // 删除已经写入的头节点
    //     _nodes.erase(begin);
    // }
    // end_input();

    // 重组完毕的 substring 再输入到 _output 中
    // 判断是否超过的 capacity
    size_t sz = data.size();
    if (eof) {
        _eof = eof;  // 不管怎么样，先搞定 eof
    }
    // 当前字符串的末尾序号已经被重组了，则直接返回
    if (sz == 0 || index + sz < _head_index) {
        end_input();
        return;
    }
    node cur;
    if (index < _head_index) {
        // 截取有用的后半部分
        size_t offset = _head_index - index;
        cur.begin = _head_index;
        cur.length = sz - offset;
        cur.data = data.substr(offset);
    } else {
        cur.begin = index;
        cur.data = data;
        cur.length = sz;
    }
    _unassembled_bytes += cur.length;
    // 合并字节
    // 二分查找，返回第一个大于等于 cur_node.begin 的节点，如果没找到，返回末尾的迭代器位置
    // 向后合并，cur.begin = 90  next.begin = 100
    long merged_bytes = 0;
    while (true) {
        auto next = _nodes.lower_bound(cur);
        if (next == _nodes.end()) {
            break;
        }
        // 合并 cur 和 next
        long m = merge(cur, *next);
        if (m < 0) {
            // 无合并
            break;
        }
        // 有合并，删除已经合并的 next 节点
        _nodes.erase(next);
        merged_bytes += m;
    }
    auto iter = _nodes.lower_bound(cur);
    // 向前合并
    // 如果已经是第一个节点了，同时也是当前节点，则已经不用向前合并了
    while (iter != _nodes.begin()) {
        // 否则 iter--，得到前面一个节点进行合并
        iter--;
        long m = merge(cur, *iter);
        if (m < 0) {
            // 无合并
            break;
        }
        // 有合并，删除已经合并的 next 节点
        _nodes.erase(iter);
        merged_bytes += m;
        iter = _nodes.lower_bound(cur);
    }
    _unassembled_bytes -= merged_bytes;
    // 插入 cur
    _nodes.insert(cur);
    while (!_nodes.empty() && _nodes.begin()->begin == _head_index) {
        auto begin = _nodes.begin();
        size_t written = _output.write(begin->data);
        _head_index += written;
        _unassembled_bytes -= written;
        // 删除已经写入的头节点
        _nodes.erase(begin);
    }
    end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const {
    // `true` if no substrings are waiting to be assembled
    // 当没有子字符串等待重组的时候返回 true
    return _nodes.empty() && _unassembled_bytes == 0;
}

size_t StreamReassembler::head_index() const { return _head_index; }

void StreamReassembler::end_input() {
    // 输出完毕，且数据为空
    // 不再往里面灌了，灌的数据都整合了
    if (_eof && empty()) {
        _output.end_input();
    }
}

long StreamReassembler::merge(node &node1, const node &node2) {
    // 合并两个 node，合并的结果为 node1，return 合并的字节数
    node l, r;  // 左、右节点
    if (node1.begin < node2.begin) {
        l = node1;
        r = node2;
    } else {
        l = node2;
        r = node1;
    }
    // 对于 l 和 r 有如下几种情况：
    // 1. r.begin + r.length <= l.begin + l.length，即 l 包含了 r，此时直接返回 l，合并的字节数是 r.length
    // 2. r.begin < l.begin + l.length && r.begin + r.length > l.begin + l.length，即 l 和 r 有交集，但 r 仍有突出部分
    // 3. l.begin + l.length < r.begin，即 l 和 r 无交集，则此时无需合并，所以返回 0

    // l 和 r 完全无交集
    if (l.begin + l.length < r.begin) {
        return -1;  // 没有交集返回负数，外层必须根据 <0 来判断，其它情况均可以合并
    }
    // l 包含 r
    if (r.begin + r.length <= l.begin + l.length) {
        node1 = l;                           // l 是合并后的节点
        return static_cast<long>(r.length);  // 合并字节数是 r 的长度
    }
    // 其它情况，则 l 和 r 都有交集
    node1.begin = l.begin;
    node1.length = r.begin + r.length - l.begin;
    long gap = l.begin + l.length - r.begin;  // 注意：当 l,r 刚好相接的时候，l 和 r 可以直接合并，但返回的却是 0
    node1.data = l.data + r.data.substr(gap);
    return gap;
}