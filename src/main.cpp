#include <iostream>
#include "seeker/logger.h"
#include "UdpServer.h"
#include "UdpClient.h"
#include <thread>
#include "cxxopts.hpp"

using std::string;
using std::thread;

cxxopts::ParseResult parse(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    seeker::Logger::init();
    auto result = parse(argc, argv);

    if (result.count("s")) {
        int port = result["p"].as<int>();
        // todo: start a server
    } else if (result.count("c")) {
        string host = result["c"].as<string>();
        int port = result["p"].as<int>();
        int time = result["t"].as<int>();
        int reportInterval = result["i"].as<int>();
        if (result.count("b")) {
            string bandwidth = result["b"].as<string>();
            char bandwidthUnit = bandwidth.at(bandwidth.size() - 1);
            auto bandwidthValue = std::stoi(bandwidth.substr(0, bandwidth.size() - 1));
            switch (bandwidthUnit) {
                case 'b':
                case 'B':
                case 'k':
                case 'K':
                case 'm':
                case 'M':
                case 'g':
                case 'G': {
                    int packetSize = 1400;
                    //todo:start bandwidth test
                    break;
                }
                default:
                    E_LOG("params error, bandwidth unit should only be B, K, M or G");
                    return -1;
            }
        } else {
            //todo:start rtt test
        }

    } else {
        E_LOG("params error, it should not happen.");
    }
    return 0;
}
