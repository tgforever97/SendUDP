//
// Created by wang on 2020/5/19.
//
#include "config.h"
#include <seeker/loggerApi.h>
#include <UdpClient.h>

UdpClient::UdpClient(const string& ip, int port) {
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    memset(buffer, 0, sizeof(buffer));

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        E_LOG("create socket error");
        throw std::runtime_error("can not create socket !");
    }
}

int UdpClient::sendAndRecv() {
    auto addrLen = (socklen_t)(sizeof(serverAddr));
    for(;;){
        auto sendLen = sendto(fd, sendBuffer, sizeof(sendBuffer), 0, (struct sockaddr *)&serverAddr, addrLen);
        if(sendLen < 0){
            E_LOG("send msg error!");
            return 0;
        }
        clock_t sendTime = clock();
        I_LOG("send msg to server msgLen={}", sendLen);
        auto recvLen = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
        if(recvLen < 0)
        {
            E_LOG("recv msg error!");
            return 0;
        }
        clock_t recvTime = clock();
        I_LOG("recv msg from server msgLen={}", recvLen);
        I_LOG("time around is {} ms", static_cast<double>(recvTime-sendTime)/CLOCKS_PER_SEC*1000000);
    }
}

void UdpClient::freeAll() {
    free(&serverAddr);
    free(buffer);
    close(fd);
}


