#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

//#include "radiotap_iter.h"
#include "radiotap_header.h"

// Uncomment if unneeded
//#define PCAP_FILE_HEADER_PRINT
#define PCAP_PACKET_HEADER_PRINT
//#define RADIOTAP_HEADER_PRINT
#define WIFI_HEADER_PRINT

struct pcap_file_header
{
    std::uint32_t pcap_magic;
    std::uint16_t pcap_major_ver;
    std::uint16_t pcap_minor_ver;
    std::uint32_t tz_offset;
    std::uint32_t timestamp_acc;
    std::uint32_t snapshot_length;
    std::uint32_t linklayer_header_type;
};

struct pcap_packet_header
{
    std::uint32_t sec;
    std::uint32_t msec;
    std::uint32_t len;
    std::uint32_t len_untrunc;
};

struct mac_address
{
    std::array<std::uint8_t, 6> addr;
    auto begin() { return addr.begin(); };
};

std::ostream& operator<<(std::ostream& os, const mac_address &mac)
{
    // Please do not judge me for this line
    return os << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned short>(mac.addr[0]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[1]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[2]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[3]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[4]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[5]) << std::dec;
}

struct wifi_packet
{
    std::uint16_t frame_control;
    std::uint16_t duration;
    mac_address addr1;
    mac_address addr2;
    /* This is unneeded for now
    mac_address addr3;
    std::uint16_t sequence_control;
    mac_address addr4;
    std::uint16_t qos_control;
    std::uint32_t ht_control;
    std::vector<std::uint8_t> raw_data;
    std::uint32_t fcs;
    */
};

int sockfd;

void pcap_parse_loop(std::string filename)
{

    pcap_file_header pcap_header;

    std::ifstream pcap_input(filename, std::ios::binary);

    pcap_input.read(reinterpret_cast<char*>(&pcap_header.pcap_magic), sizeof pcap_header.pcap_magic);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.pcap_major_ver), sizeof pcap_header.pcap_major_ver);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.pcap_minor_ver), sizeof pcap_header.pcap_minor_ver);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.tz_offset), sizeof pcap_header.tz_offset);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.timestamp_acc), sizeof pcap_header.timestamp_acc);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.snapshot_length), sizeof pcap_header.snapshot_length);
    pcap_input.read(reinterpret_cast<char*>(&pcap_header.linklayer_header_type), sizeof pcap_header.linklayer_header_type);

#ifdef PCAP_FILE_HEADER_PRINT
    std::cout << "-- pcap file header --\nMagic number: 0x" << std::setfill('0') << std::setw(8) << std::hex << pcap_header.pcap_magic << std::endl;
    std::cout << std::dec;
    std::cout << "Major ver: " << pcap_header.pcap_major_ver << std::endl;
    std::cout << "Minor ver: " << pcap_header.pcap_minor_ver << std::endl;
    std::cout << "Time zone offset: " << pcap_header.tz_offset << std::endl;
    std::cout << "Time stamp accuracy: " << pcap_header.timestamp_acc << std::endl;
    std::cout << "Snapshot length: " << pcap_header.snapshot_length << std::endl;
    std::cout << "Link-layer header type: " << pcap_header.linklayer_header_type << std::endl;
#endif

    switch (pcap_header.linklayer_header_type)
    {
        case 127: // WiFi
        {
            while (pcap_input.peek() != EOF)
            {
                pcap_packet_header packet_header;

                pcap_input.read(reinterpret_cast<char*>(&packet_header.sec), sizeof packet_header.sec);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.msec), sizeof packet_header.msec);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.len), sizeof packet_header.len);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.len_untrunc), sizeof packet_header.len_untrunc);

                std::vector<std::uint8_t> packet_data;
                packet_data.resize(packet_header.len);
                pcap_input.read(reinterpret_cast<char*>(packet_data.data()), packet_header.len);

#ifdef PCAP_PACKET_HEADER_PRINT
                std::cout << std::dec;
                std::cout << "-- pcap packet_header --\nsec: " << packet_header.sec << std::endl;
                std::cout << "msec: " << packet_header.msec << std::endl;
                std::cout << "Data length: " << packet_header.len << std::endl;
                std::cout << "Data length (if untruncted): " << packet_header.len_untrunc << std::endl;
#endif

                struct ieee80211_radiotap_header rt_header;
                //struct ieee80211_radiotap_vendor_namespaces rt_vns;

                rt_header.it_version = packet_data[0];
                rt_header.it_pad = packet_data[1];
                rt_header.it_len = (packet_data[3]<<8) + packet_data[2];
                rt_header.it_present = (packet_data[7]<<24) + (packet_data[6]<<16) + (packet_data[5]<<8) + packet_data[4];

#ifdef RADIOTAP_HEADER_PRINT
                std::cout << " -- Radiotap header --\nVersion: " << std::to_string(rt_header.it_version) << std::endl;
                std::cout << "Pad: " << std::to_string(rt_header.it_pad) << std::endl;
                std::cout << "Length: " << std::to_string(rt_header.it_len) << std::endl;
                std::cout << "Present: " << std::bitset<32>(rt_header.it_present) << " | 0x" << std::hex << rt_header.it_present << std::endl;
#endif

/*
                struct ieee80211_radiotap_iterator rt_iterator;
                if (!ieee80211_radiotap_iterator_init(&rt_iterator, reinterpret_cast<ieee80211_radiotap_header*>(packet_data.data()), 9999, &vns))
                {
                    std::cout << "Radiotap iterator init failed!" << std::endl;
                    exit(2);
                }
*/

                wifi_packet packet;
                
                auto wifi_data = packet_data.begin()+rt_header.it_len;

                packet.frame_control = (wifi_data[0]<<8)+wifi_data[1];
                packet.duration = (wifi_data[2]<<8)+wifi_data[3];
                std::copy(wifi_data+4, wifi_data+10, packet.addr1.begin());
                std::copy(wifi_data+10, wifi_data+16, packet.addr2.begin());

#ifdef WIFI_HEADER_PRINT
                std::cout << "-- WiFi header --\nFrame control: " << packet.frame_control << std::endl;
                std::cout << "Duration: " << packet.duration << std::endl;
                std::cout << "Address 1 (receiver): " << packet.addr1 << std::endl;
                std::cout << "Address 2 (transmitter): " << packet.addr2 << std::endl;
#endif

                char msg_data[16] = {'\0'};                
                msg_data[0] = 1; // Frame code
                for (std::uint8_t i = 0; i < 6; i++)
                    msg_data[1+i] = packet.addr2.addr[i];
                for (std::uint8_t i = 0; i < 4; i++)
                    msg_data[7+i] = *(reinterpret_cast<std::uint8_t*>(&packet_header.sec)+(3-i));
                for (std::uint8_t i = 0; i < 4; i++)
                    msg_data[11+i] = *(reinterpret_cast<std::uint8_t*>(&packet_header.msec)+(3-i));
                msg_data[15] = packet_data[22];

                int bytes_sent = send(sockfd, msg_data, 16, 0);

                if (bytes_sent == 0)
                {
                    std::cout << "Connection closed!" << std::endl;
                    exit(0);
                }

                if (bytes_sent != 16)
                {
                    std::cout << "Failed to send packet!" << std::endl;
                }

            }
            break;
        }

        default:
            std::cout << "Unhandled packet type " << pcap_header.linklayer_header_type << " from file " << filename << std::endl;
            exit(1);
            break;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <slave number>" << std::endl;
        exit(1);
    }

    // Connect to master
    std::cout << "Connecting to master ..." << std::endl;

    struct addrinfo hints = {};
    struct addrinfo *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("192.168.10.2", "4443", &hints, &res);

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        std::cout << "Failed to open socket!" << std::endl;
        exit(1);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        std::cout << "Failed to connect!" << std::endl;
        exit(2);
    }

    std::cout << "Connected to master" << std::endl;

    std::uint8_t slave_number = argv[1][0]-'0';

    if (send(sockfd, &slave_number, 1, 0) != 1)
    {
        std::cout << "Failed to send slave number" << std::endl;
        exit(3);
    }

    std::thread wifi_thread;

    pcap_parse_loop("wifi_pcap_pipe"); 

    return 0;
}
