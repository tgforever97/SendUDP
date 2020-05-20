#include <seeker/loggerApi.h>
#include "config.h"
#include "UdpServer.h"

using std::runtime_error;

UdpServer::UdpServer(int port) {
    memset((char *)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(port);
    memset(buffer, 0, sizeof(buffer));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        E_LOG("cannot create socket\n");
        throw std::runtime_error("can not create socket !");
    }

    if ((bind(fd, (struct sockaddr *) &myAddr, sizeof(myAddr))) < 0) {
        E_LOG("bind failed");
        throw std::runtime_error("can not bind !");
    }
}

int UdpServer::recvAndSend() {
    auto len = sizeof(remAddr);
    for(;;){
        auto recvLen = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remAddr,
                                 (socklen_t *)&len);
        if(recvLen < 0){
            E_LOG("recvfrom error:");
            return 0;
        }
        I_LOG("recv msg from client msgLen={}", recvLen);
        auto sendLen = sendto(fd, sendMsg, sizeof(sendMsg), 0, (struct sockaddr *)&remAddr, len);
        if(sendLen < 0){
            E_LOG("sendto error:");
            return 0;
        }
        I_LOG("send msg to client msgLen={}", sendLen);
    }
}

void UdpServer::freeAll() {
    free(&myAddr);
    free(buffer);
    close(fd);
}

//UdpServer::~UdpServer() {
////    free(&myAddr);
//    free(buffer);
//    close(fd);
//}
