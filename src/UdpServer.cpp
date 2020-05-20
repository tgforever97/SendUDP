#include <seeker/loggerApi.h>
#include "config.h"
#include "UdpServer.h"
#include "Message.h"

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

void UdpServer::freeAll() {
    close(fd);
}

void UdpServer::rttServer() {
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
                TestConfirm response(1, testIdGen.fetch_add(1), req.msgId, Message::genMid());
                I_LOG("Reply Msg TestConfirm, msgId={}, testType={}, rst={}", response.msgId,
                      (int)response.msgType, response.result);
                Message::sendMsg(response, fd, (struct sockaddr *)&remAddr, len);
                break;
            }
            case MessageType::rttTestMsg: {
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



