#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
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

    bool operator<(const struct mac_address &mac) const
    {
        return key() < mac.key();
    }

    std::uint64_t key() const
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
    double avg_str = -100000000;
    // Timestamp?

    bool operator==(const struct device &dev) const
    {
        return mac.key() == dev.mac.key();
    }

    bool operator<(const struct device &dev) const
    {
        return avg_str < dev.avg_str;
    }
};

const auto& map_comp = [](std::pair<struct mac_address, struct device> elem1, std::pair<struct mac_address, struct device> elem2)
{
    return elem1.second.avg_str > elem2.second.avg_str;
};

const auto& set_comp = [](const struct device elem1, const struct device elem2)
{
    return elem1.avg_str > elem2.avg_str;
};

std::map<struct mac_address, struct device> devices;

void slave_listen(const struct slave &slv)
{
    while (true)
    {
        char buffer[BUFFER_SIZE];

        int bytes_recv = recv(slv.sockfd, buffer, 16, 0);

        if (bytes_recv == 0)
        {
            std::cout << "Connection closed to slave " << static_cast<unsigned short>(slv.slave_number) << "!" << std::endl;
            exit(0);
        }

        if (bytes_recv != 16 || buffer[0] != 1)
        {
            std::cout << "Wrong message recv! byte count = " << bytes_recv << " first bit = "  << static_cast<unsigned short>(buffer[0]) << std::endl;
            continue;
        }

        //std::cout << "Recived update with signal strength " << std::to_string(*reinterpret_cast<signed char*>(&buffer[15])) << std::endl; 

        struct device dev = {};

        for (std::uint8_t i = 0; i < 6; i++)
        {
            dev.mac.addr[i] = buffer[1+i];
        }

        //std::cout << "Received update on device " << new_dev.mac << " " << std::hex << new_dev.mac.key() << std::dec << std::endl;

        if (devices.count(dev.mac) == 1)
        {
            dev = devices.at(dev.mac);
        }

        dev.sig_str[slv.slave_number] = *reinterpret_cast<int8_t*>(&buffer[15]);
        dev.avg_str = 0;
        for (double str : dev.sig_str)
        {
            dev.avg_str += str;
        }
        dev.avg_str /= dev.sig_str.size();

        //std::cout << "Received update on device " << dev.mac << " " << std::hex << dev.mac.key() << std::dec << std::endl;

        devices[dev.mac] = dev;
    }
}

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
    std::array<slave, SLAVE_COUNT> slaves;

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

    std::array<std::thread, SLAVE_COUNT> threads;

    for (std::uint8_t i = 0; i < SLAVE_COUNT; i++)
    {
        threads[i] = std::thread(slave_listen, slaves[i]);
    }

    while (true)
    {
        std::cout << "Device count = " << std::dec << devices.size() << std::endl;
        std::uint8_t i = 20;

        std::set<std::pair<struct mac_address, struct device>, decltype(map_comp)> devices_sorted(devices.begin(), devices.end(), map_comp);

        for (const auto& dev : devices_sorted)
        {
            if (dev.second.avg_str > -100 || true)
            {
                std::cout << dev.second.mac << ' ' << dev.second.avg_str << std::endl;

                i--;

                if (i == 0)
                    break;
            } 
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
