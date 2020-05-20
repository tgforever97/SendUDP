#include <iostream>
#include "seeker/logger.h"
#include "UdpServer.h"
#include "UdpClient.h"
#include <thread>

using std::string;
using std::thread;

int main() {
    seeker::Logger::init();
    std::cout << "UDP TEST!" << std::endl;
    auto server = UdpServer(8888);
    auto client = UdpClient("127.0.0.1", 8888);
//    server.recvAndSend();
    thread t1 = thread(&UdpServer::recvAndSend, server);
    thread t2 = thread(&UdpClient::sendAndRecv, client);
    t1.join();
    t2.join();
    server.freeAll();
    client.freeAll();
    return 0;
}
