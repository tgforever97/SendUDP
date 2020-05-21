#include "config.h"
#include <seeker/loggerApi.h>
#include "UdpServer.h"
#include "Message.h"

using std::runtime_error;

UdpServer::UdpServer(int port) {
    memset((char *)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        E_LOG("cannot create socket\n");
        throw std::runtime_error("can not create socket !");
    }

    if ((bind(fd, (struct sockaddr *) &myAddr, sizeof(myAddr))) < 0) {
        E_LOG("bind failed");
        throw std::runtime_error("can not bind !");
    }
}

void UdpServer::freeAll() {
    close(fd);
}

void UdpServer::startServer() {
    I_LOG("udp server is started...");
    uint8_t buffer[BUFSIZE];
    auto len = sizeof(remAddr);
    for(;;){
        auto recvLen = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remAddr,
                                (socklen_t *)&len);
        if(recvLen < 0){
            E_LOG("recvfrom error:");
            exit(0);
        }
        uint8_t msgType;
        Message::getMsgType(buffer, msgType);
        int msgId = 0;
        Message::getMsgId(buffer, msgId);
        switch ((MessageType)msgType) {
            case MessageType::testRequest: {
                TestRequest req(buffer);
                I_LOG("Got TestRequest, msgId={}, testType={}", req.msgId, (int)req.testType);
                TestConfirm response(1, nextTestId.fetch_add(1), req.msgId, Message::genMid());
                I_LOG("Reply Msg TestConfirm, msgId={}, testType={}, rst={}", response.msgId,
                      (int)response.msgType, response.result);
                Message::sendMsg(response, fd, (struct sockaddr *)&remAddr, len);
                if(req.testType == (int)TestType::bandwidth){
                    bandWidthServer();
                }
                break;
            }
            case MessageType::rttTestMsg: {
                I_LOG("start to test rtt...");
                auto sendLen = sendto(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remAddr, len);
                if(sendLen < 0){
                    E_LOG("sendto error:");
                    throw std::runtime_error("sendro failed");
                }
                D_LOG("Reply rttTestMsg, msgId={}", msgId);
                break;
            }
            default:
                W_LOG("Got unknown msg, ignore it. msgType={}, msgId={}", msgType, msgId);
                break;
        }
        memset(buffer, 0, recvLen);
    }
}

void UdpServer::bandWidthServer() { //带宽，丢包，抖动
    I_LOG("start to test bandWidth...");
    uint8_t buffer[BUFSIZE];
    memset(buffer, 0 ,BUFSIZE);
    auto len = sizeof(remAddr);

    uint64_t recvByte = 0;
    int64_t maxDelay = 0;
    int64_t minDelay = 0;
    int64_t delay = 0;
    int packetCount = 0;
    int64_t lastArrivalTime = 0;
    int64_t startTime = -1;

    for(;;){
        auto recvLen = recvfrom(fd, buffer, BUFSIZE, 0, (struct sockaddr *)&remAddr,
                                (socklen_t *)&len);
        if(recvLen < 0){
            E_LOG("recvfrom error:");
            throw std::runtime_error("recvffrom error..");
        }
        uint8_t msgType;
        Message::getMsgType(buffer, msgType);
        int msgId;
        Message::getMsgId(buffer, msgId);
        uint16_t testId;
        Message::getTestId(buffer, testId);
        int64_t timestamp;
        Message::getTimestamp(buffer, timestamp);
        int testNum;
        BandwidthTestMsg::getTestNum(buffer, testNum);

        if(msgType == (uint8_t)MessageType::bandwidthTestMsg){
            I_LOG("receive a BandwidthTestMsg, testNum={}", testNum);
            recvByte += recvLen;

            lastArrivalTime = seeker::Time::currentTime();
            if(startTime < 0){
                startTime = lastArrivalTime;
            }

            delay = seeker::Time::microTime() - timestamp;
            if (delay < minDelay) minDelay = delay;
            if (delay > maxDelay) maxDelay = delay;

            packetCount++;
        }else if(msgType == (uint8_t)MessageType::bandwidthFinish){
            int totalPacket;
            BandwidthFinish::getTotalPkt(buffer, totalPacket);
            int lossPkt = totalPacket - packetCount;
            int64_t jitter = maxDelay - minDelay;
            I_LOG("bandwidth test report:");
            I_LOG("[{}]  {}    {}bytes/s     {}ms   {}/{} ({:.{}f}%)",
                  testId,
                  recvByte,
                  recvByte*1000/(lastArrivalTime - startTime),
                  (double)jitter / 1000,
                  lossPkt,
                  packetCount,
                  (double)100 * lossPkt / totalPacket,
                  4);
            BandwidthReport report(jitter, packetCount, recvByte, testId, Message::genMid());
            Message::sendMsg(report, fd, (struct sockaddr *)&remAddr, len);
            break;
        }else{
            W_LOG("Got a unexpected msg. msgId={} msgType={}", msgId,msgType);
        }
    }
    I_LOG("bandwidth finished");
}





