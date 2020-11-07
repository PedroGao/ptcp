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
    // DUMMY_CODE(data, index, eof);
    // 是否小于 _head_index
    // 通过 set 找到lower 合并
    // 再检查能够向前合并
    // 合并完成以后检查能否插入进  _output
    // 如果当前的序号大于 头节点的序号 + 容量，表示越界了，所以直接返回
    // 这里 _head_index 表示已经第一个字节的序号
    // if (index >= _head_index + _capacity) {
    //     return;
    // }
    // lower_bound
    // 二分查找一个有序数列，返回第一个大于等于x的数，如果没找到，返回末尾的迭代器位置
    node elem;
    // 如果当前序号 + 数据长度 <= 头部序号，则表示太小
    // 注意：这里的 _head_index 是从 0 开始的，表示 merge 一定会从 0 开始合并
    // 如果有其它节点则不会合并
    if (index + data.length() <= _head_index) {
        goto JUDGE_EOF;
    } else if (index < _head_index) {
        // 如果当前的序号小于头部序号，因为 _head_index 之前的 bytes 全部都被合并了
        // 所以截取掉前端的 offset 字节
        size_t offset = _head_index - index;
        elem.data.assign(data.begin() + offset, data.end());
        elem.begin = index + offset;
        elem.length = elem.data.length();
    } else {
        // 如果当前的序号大于 _head_index，则直接将 data 赋值给 ele
        elem.begin = index;
        elem.length = data.length();
        elem.data = data;
    }
    // 新来的节点都是没有被重组的
    _unassembled_byte += elem.length;
    // 合并节点
    do {
        long merged_bytes = 0;
        // 向前合并，通过 lower_bound 得到 begin 大于等于 elem begin 的节点
        auto iter = _blocks.lower_bound(elem);
        // 如果不是最后一个，且 elem 能与 iter 合并
        while (iter != _blocks.end() && (merged_bytes = merge_node(elem, *iter)) >= 0) {
            // 节点重组
            _unassembled_byte -= merged_bytes;
            // iter 被合并，所以删除
            _blocks.erase(iter);
            // 找到下一个 iter
            iter = _blocks.lower_bound(elem);
        }
        // 合并之前的节点
        if (iter == _blocks.begin()) {
            // 如果已经是第一个节点，即只剩下一个节点
            break;
        }
        // 合并完后面的之后，遇到 end 或者 二个节点没有交集，则将 iter-- ，得到 iter 前一个节点，看是否能合并
        iter--;
        while ((merged_bytes = merge_node(elem, *iter)) >= 0) {
            _unassembled_byte -= merged_bytes;
            _blocks.erase(iter);
            // iter 被合并了，所以要删除它，但是还需要得到 >= elem.begin 的节点
            iter = _blocks.lower_bound(elem);
            // 如果到达了头部
            if (iter == _blocks.begin()) {
                break;
            }
            iter--;
        }
    } while (false);
    // 插入这个合并节点
    _blocks.insert(elem);

    // 写入数据
    if (!_blocks.empty() && _blocks.begin()->begin == _head_index) {
        const node head = *_blocks.begin();
        size_t write_bytes = _output.write(head.data);
        _head_index += write_bytes;
        _unassembled_byte -= write_bytes;
        _blocks.erase(_blocks.begin());
    }

JUDGE_EOF:
    if (eof) {
        _eof = true;
    }
    // 如果当前 eof 且全部都已经重组了，设置 _output
    if (_eof && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_byte; }

bool StreamReassembler::empty() const { return _unassembled_byte == 0; }

long StreamReassembler::merge_node(node &node1, const node &node2) {
    node x, y;
    // 决定 node1 和 node2 谁在前面
    if (node1.begin > node2.begin) {
        x = node2;
        y = node1;
    } else {
        x = node1;
        y = node2;
    }
    // 如果 x 的 最右边 仍然在 y 的左边，则二者根本没有交集，所以直接返回 -1
    if (x.begin + x.length < y.begin) {
        return -1;
    } else if (x.begin + x.length >= y.begin + y.length) {
        // 如果 x 的右边在 y 右边的 右边，所以 y 直接被 x 覆盖，因此直接返回 y 的长度
        // 故 y 的数据是多余的
        node1 = x;
        return y.length;
    } else {
        // 这个时候 x 的右边大于 y 的左边，但小于 y 的右边
        node1.begin = x.begin;
        node1.data = x.data + y.data.substr(x.begin + x.length - y.begin);
        node1.length = node1.data.length();
        return x.begin + x.length - y.begin;
    }
}
