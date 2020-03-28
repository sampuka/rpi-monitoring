#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 100
#define SLAVE_COUNT 1

struct slave
{
    int sockfd;
    std::uint8_t slave_number;
};

struct mac_address
{
    std::array<std::uint8_t, 6> addr;
    std::uint64_t key()
    {
        std::uint64_t key = 0;
        for (std::uint8_t i = 0; i < 6; i++)
        {
            key <<= 8;
            key += addr[i];
        }
        return key;
    }
};

std::ostream& operator<<(std::ostream& os, const mac_address &mac)
{
    // Please do not judge me for this line
    return os << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned short>(mac.addr[0]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[1]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[2]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[3]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[4]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[5]);
}

struct vec
{
    double x;
    double y;
};

struct device
{
    struct mac_address mac;
    struct vec pos;
    std::array<double, SLAVE_COUNT> sig_str;
    // Timestamp?
};

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // Set up server
    std::cout << "Setting up server ..." << std::endl;

    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *res;

    const char *port = "4443";

    getaddrinfo(NULL, port, &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        std::cout << "Failed to open socket!" << std::endl;
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        std::cout << "Failed to bind socket!" << std::endl;
        exit(2);
    }

    listen(sockfd, 5);

    std::cout << "Server listening on port " << port << std::endl;

    // Connect to slaves
    std::vector<slave> slaves;
    slaves.resize(SLAVE_COUNT);

    for (std::uint8_t i = 0; i < SLAVE_COUNT; i++) 
    {
        struct sockaddr_storage cli_addr;
        socklen_t clilen = sizeof cli_addr;
        int cli_sockfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&cli_addr), &clilen);

        if (cli_sockfd < 0)
        {
            std::cout << "Failed to accept on socket!" << std::endl;
            exit(3);
        }

        char buffer[BUFFER_SIZE];
        int bytes_recv = recv(cli_sockfd, &buffer, BUFFER_SIZE, 0);

        if (bytes_recv != 1)
        {
            std::cout << "Client didn't send slave number byte as first message!" << std::endl;
            exit(4);
        }

        const std::uint8_t slave_number = buffer[0];

        std::cout << "Connected to slave " << static_cast<unsigned short>(slave_number) << std::endl;

        slaves[i].sockfd = cli_sockfd;
        slaves[i].slave_number = slave_number;
    }

    std::unordered_map<std::uint64_t, struct device> devices;

    devices[1] = {};

    return 0;
}
