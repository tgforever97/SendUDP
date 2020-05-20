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

    memset(buffer, 0, BUFSIZE);
    memset(sendBuffer, 0, BUFSIZE);

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

void UdpClient::bandWidthClient(uint32_t bandwidth, char bandwidthUnit, int packetSize, int testSeconds) {
    auto addrLen = (socklen_t)(sizeof(serverAddr));
    TestRequest req(TestType::bandwidth, 0, Message::genMid());
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

    uint64_t bandWidthReal = 0;
    switch (bandwidthUnit){
        case 'b':
        case 'B':
            bandWidthReal = bandwidth;
            break;
        case 'k':
        case 'K':
            bandWidthReal = 1024 * (uint64_t)bandwidth;
            break;
        case 'm':
        case 'M':
            bandWidthReal = 1024 * 1024 * (uint64_t)bandwidth;
            break;
        case 'g':
        case 'G':
            bandWidthReal = 1024 * 1024 * 1024 * (uint64_t)bandwidth;
            break;
        default:
            throw std::runtime_error("error bandWidthUnit.." + std::to_string(bandwidthUnit));
    }

    bandWidthReal /= 8; //to byte

    auto packetsPerSecond = bandWidthReal / packetSize;
    auto packetsSendInterval = 1000 / packetsPerSecond;
    auto totalPackets = packetsPerSecond * testSeconds;
    I_LOG("bandwidthValue={}, packetsPerSecond={}, packetsendInterval={}, totalPackets={}",
          bandWidthReal,
          packetsPerSecond,
          packetsSendInterval,
          totalPackets);

    BandwidthTestMsg msg(packetSize, testId, 0, Message::genMid());
    msg.getBinary(sendBuffer, MSG_SEND_BUF_SIZE);

    auto totalLen = 0;

    for(int i = 0; i < totalPackets; i++){
        int64_t startTime = seeker::Time::currentTime();
        BandwidthTestMsg::update(sendBuffer, Message::genMid(), i, seeker::Time::microTime());
        auto sendLen = sendto(fd, sendBuffer, msg.getLength(), 0, (struct sockaddr *)&serverAddr, addrLen);
        if(sendLen < 0){
            E_LOG("send bandWidthmsg error..");
            throw std::runtime_error("send bandWidthmsg error..");
        }
        totalLen += sendLen;
        int64_t processTime = seeker::Time::currentTime() - startTime;
        std::this_thread::sleep_for(std::chrono::milliseconds(packetsSendInterval - processTime));
    }

}




