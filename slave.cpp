#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <vector>

// Uncomment if unneeded
#define VERBOSE

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
    return os << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned short>(mac.addr[0]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[1]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[2]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[3]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[4]) << ':' << std::setw(2) << static_cast<unsigned short>(mac.addr[5]);
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

#ifdef VERBOSE
    std::cout << "pcap file header:\nMagic number: 0x" << std::setfill('0') << std::setw(8) << std::hex << pcap_header.pcap_magic << std::endl;
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
                wifi_packet packet;

                pcap_input.read(reinterpret_cast<char*>(&packet_header.sec), sizeof packet_header.sec);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.msec), sizeof packet_header.msec);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.len), sizeof packet_header.len);
                pcap_input.read(reinterpret_cast<char*>(&packet_header.len_untrunc), sizeof packet_header.len_untrunc);

                std::vector<std::uint8_t> packet_data;
                packet_data.resize(packet_header.len);
                pcap_input.read(reinterpret_cast<char*>(packet_data.data()), packet_header.len);

                packet.frame_control = (packet_data[0]<<8)+packet_data[1];
                packet.duration = (packet_data[2]<<8)+packet_data[3];
                std::copy(packet_data.begin()+4, packet_data.begin()+10, packet.addr1.begin());
                std::copy(packet_data.begin()+10, packet_data.begin()+16, packet.addr2.begin());

#ifdef VERBOSE
                std::cout << std::dec;
                std::cout << "-- pcap packet_header --\nsec: " << packet_header.sec << std::endl;
                std::cout << "msec: " << packet_header.msec << std::endl;
                std::cout << "Data length: " << packet_header.len << std::endl;
                std::cout << "Data length (if untruncted): " << packet_header.len_untrunc << std::endl;

                std::cout << "-- WiFi header --\nFrame control: " << packet.frame_control << std::endl;
                std::cout << "Duration: " << packet.duration << std::endl;
                std::cout << "Address 1 (receiver): " << packet.addr1 << std::endl;
                std::cout << "Address 2 (transmitter): " << packet.addr2 << std::endl;
#endif
            }
            break;
        }

        default:
            std::cout << "Unhandled packet type from file " << filename << std::endl;
            exit(1);
            break;
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    pcap_parse_loop("/dev/stdin"); 

   return 0;
}
