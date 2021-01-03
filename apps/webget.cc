#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    // Hints:
    // 1. HTTP 每一行都以 \r\n 结尾
    // 2. 不要忘记在 header 中加入：Connection: close
    // 3. 读取和输出所有服务器的数据，在 EOF 之前
    // 4. 只写 10 行代码即可
    // 5. 使用 make check_webget 检查代码是否 work

    Address address(host, "http");
    TCPSocket socket;
    // 连接远端地址
    socket.connect(address);
    string req = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\nAccept: */*\r\n\r\n";
    // 写入请求 http 数据
    socket.write(req);
    // 只写一次，直接关闭 write
    socket.shutdown(SHUT_WR);
    // 只要没有 eof 就一直读
    while (!socket.eof()) {
        cout << socket.read();
    }
    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
