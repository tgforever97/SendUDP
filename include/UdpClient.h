/**
  UDP client

  @project SendUDP
  @author WangMengYuan
  @since 2020/5/19
  @version v0.0.1 2020/5/19
*/

#ifndef SENDUDP_UDPCLIENT_H
#define SENDUDP_UDPCLIENT_H

#define BUFSIZE 2048

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using std::string;

class UdpClient{

public:
    explicit UdpClient(const string& ip, int port);

    int sendAndRecv();

    void freeAll();

private:
    struct sockaddr_in myAddr;
    struct sockaddr_in serverAddr;
    int fd;
    unsigned char buffer[BUFSIZE];
    unsigned char sendBuffer[10] = "send msg.";
};

#endif //SENDUDP_UDPCLIENT_H
