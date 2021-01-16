#include <iostream>
#include <set>
#include <stdint.h>
#include <string>

using namespace std;

struct Node {
    size_t begin = 0;
    size_t length = 0;
    std::string data{};
    //! 通过节点的开始节点来排序节点
    bool operator<(const Node &t) const { return begin < t.begin; }
};

int main(int argc, char const *argv[]) {
    set<Node> nodes;

    Node node1;
    Node node2;
    Node node3;
    node1.begin = 0;
    node1.length = 3;
    node1.data = "abc";
    node2.begin = 4;
    node2.length = 5;
    node2.data = "defgh";
    node3.begin = 9;
    node3.length = 2;
    node3.data = "ij";
    nodes.insert(node1);
    nodes.insert(node2);
    nodes.insert(node3);

    for (auto &node : nodes) {
        cout << node.data;
    }
    cout << endl;

    return 0;
}
