//
// Created by wang on 2020/5/19.
//
#include "config.h"
#include <seeker/loggerApi.h>
#include <UdpClient.h>
#include "Message.h"

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
 void UdpClient::rttClient(int testTimes, int packetSize) {
    auto addrLen = (socklen_t)(sizeof(serverAddr));
    TestRequest req(TestType::rtt, 0, Message::genMid());
    Message::sendMsg(req, fd, (struct sockaddr *)&serverAddr, addrLen);
    I_LOG("send TestRequest, msgId={} testType={}", req.msgId, req.testType);

    auto recvLen = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
    if(recvLen < 0){
         E_LOG("recv msg error!");
         throw std::runtime_error("recv msg from server..");
    }

    TestConfirm confirm(buffer);
    I_LOG("receive TestConfirm, result={} reMsgId={} testId={}",
           confirm.result,
           confirm.reMsgId,
           req.testId);
    auto testId = confirm.testId;
     memset(buffer, 0, recvLen);

    while(testTimes > 0){
        testTimes-- ;
        RttTestMsg msg(packetSize, testId, Message::genMid());
        Message::sendMsg(msg, fd, (struct sockaddr *)&serverAddr, addrLen);
        I_LOG("send RttTestMsg, msgId={} testId={} time={}", msg.msgId, msg.testId, msg.timestamp);

        recvLen = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
        if(recvLen < 0){
            E_LOG("recv msg error!");
            throw std::runtime_error("recv msg from server..");
        }
        RttTestMsg rttResMsg(buffer);
        auto diffTime = seeker::Time::microTime() - rttResMsg.timestamp;
        I_LOG("receive RttTestMsg, msgId={} testId={} time={} diff={}ms",
              rttResMsg.msgId,
              rttResMsg.testId,
              rttResMsg.timestamp,
              (double)diffTime / 1000);
    }
}

void UdpClient::freeAll() {
    free(&serverAddr);
    free(buffer);
    close(fd);
}


