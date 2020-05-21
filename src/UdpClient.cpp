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

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        E_LOG("create socket error");
        throw std::runtime_error("can not create socket !");
    }
}
 void UdpClient::rttClient(int testTimes, int packetSize) {
    uint8_t buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);
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
    close(fd);
}

void UdpClient::bandWidthClient(uint32_t bandwidth, char bandwidthUnit, int packetSize, int testSeconds) {
    uint8_t buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);
    uint8_t sendBuffer[BUFSIZE];
    memset(sendBuffer, 0, BUFSIZE);

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
    I_LOG("bandwidthValue={}, packetsPerSecond={}, packetsendInterval={}",
          bandWidthReal,
          packetsPerSecond,
          packetsSendInterval);

    BandwidthTestMsg msg(packetSize, testId, 0, Message::genMid());
    msg.getBinary(sendBuffer, MSG_SEND_BUF_SIZE);

    auto totalLen = 0;
    auto totalPackets = 0;
    int64_t passedTime = 0;

    auto startTime = seeker::Time::currentTime();

    std::thread worker(std::mem_fn(&UdpClient::sendBandWidthMsg), std::ref(*this), sendBuffer, std::ref(totalPackets), std::ref(totalLen), msg.getLength());

    while(totalPackets < testSeconds * packetsPerSecond){
        {
            std::lock_guard<std::mutex> lock(m);
            startSend = 1;
        }
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(packetsSendInterval));
        D_LOG("startTime {}", seeker::Time::currentTime());
    }
    passedTime = seeker::Time::currentTime() - startTime;

    BandwidthFinish finishMsg(testId, totalPackets, Message::genMid());
    Message::sendMsg(finishMsg, fd, (struct sockaddr *)&serverAddr, addrLen);
    recvLen = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr *)&serverAddr, &addrLen);
    if(recvLen < 0){
        E_LOG("recv msg error!");
        throw std::runtime_error("recv msg from server..");
    }
    BandwidthReport report(buffer);
    I_LOG("bandwidth test report:");
    I_LOG("[ ID] Transfer    Bandwidth     Jitter   Lost/Total Datagrams");
    int lossPkt = totalPackets - report.receivedPkt;
    I_LOG("[{}]  {}    {}bytes/s     {}ms   {}/{} ({:.{}f}%)",
          testId,
          totalLen,
          totalLen/passedTime,
          (double)report.jitterMicroSec / 1000,
          lossPkt,
          totalPackets,
          (double)100 * lossPkt / totalPackets,
          4);
    {
        std::lock_guard<std::mutex> lock(m);
        startSend = -1;
    }
    cv.notify_one();

    worker.join();
}

void UdpClient::test() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [this]{return startSend == 1;});
    std::cout << "teset" << std::endl;
}

void UdpClient::sendBandWidthMsg(uint8_t *buffer, int &totalPackets, int &totalLen, size_t msgLen) {
    std::unique_lock<std::mutex> lk(m);
    while(true){
        cv.wait(lk, [this]{return startSend == 1 || startSend == -1;});
        if(startSend == -1){
            break;
        }
        BandwidthTestMsg::update(buffer, Message::genMid(), totalPackets, seeker::Time::microTime());
        auto sendLen = sendto(fd, buffer, msgLen, 0, (struct sockaddr *)&serverAddr, (socklen_t) sizeof(serverAddr));
        if(sendLen < 0){
            E_LOG("send bandWidthmsg error..");
            throw std::runtime_error("send bandWidthmsg error..");
        }
        totalLen += sendLen;
        totalPackets += 1;
        startSend = 0;
    }
    I_LOG("finsh send..");
}






