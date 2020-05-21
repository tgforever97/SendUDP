#include "config.h"
#include "cxxopts.hpp"
#include <string>


using std::string;

cxxopts::ParseResult parse(int argc, char* argv[]) {
    cxxopts::Options options(argv[0], "SendUDP help to test net.");
    try {

        options
           .positional_help("[-s | -c <host>] [options")
           .show_positional_help();

        options.allow_unrecognised_options().add_options()
                ("s,server", "run in server mode")
                ("c,client", "run in client mode, connecting to <host>", cxxopts::value<string>(),"<host>")
                ("p,port", "server port to listen on/connect to", cxxopts::value<int>()->default_value("8888"), "<port>")
                ("r,rtt", "rtt test", cxxopts::value<bool>())
                ("b,bandwidth", "target bandwidth in Kbits/sec", cxxopts::value<string>()->default_value("1M"))
                ("i,interval", "seconds between periodic bandwidth reports", cxxopts::value<int>()->default_value("1"), "<sec>")
                ("t,time", "time in seconds to transmit for", cxxopts::value<int>()->default_value("10"), "<sec>")
                ("d", "test rtt and bandWidth both")
                ("h,help", "show this help");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({""}) << std::endl;
            exit(0);
        }

        if (!result.count("s") && !result.count("c")) {
            std::cout << options.help({""}) << std::endl;
            exit(0);
        }

        return result;

    } catch (const cxxopts::OptionException& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        std::cout << options.help({""}) << std::endl;
        exit(1);
    }
}
