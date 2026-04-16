#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <arpa/inet.h>

// TCP flags (same bit positions as real TCP)
static constexpr uint8_t FLAG_FIN = 0x01;
static constexpr uint8_t FLAG_SYN = 0x02;
static constexpr uint8_t FLAG_RST = 0x04;
static constexpr uint8_t FLAG_ACK = 0x10;

static constexpr int    SERVER_PORT  = 8080;
static constexpr int    MAX_PAYLOAD  = 256;

// Simulated TCP packet sent over UDP
struct TCPPacket {
    uint32_t seq;       // Sequence number
    uint32_t ack;       // Acknowledgment number
    uint8_t  flags;     // SYN / ACK / FIN / RST
    uint16_t data_len;  // Payload length
    char     data[MAX_PAYLOAD];

    // Serialize to network byte order before sending
    void hton() {
        seq      = htonl(seq);
        ack      = htonl(ack);
        data_len = htons(data_len);
    }

    // Deserialize from network byte order after receiving
    void ntoh() {
        seq      = ntohl(seq);
        ack      = ntohl(ack);
        data_len = ntohs(data_len);
    }
};

inline std::string flags_to_str(uint8_t flags) {
    std::string s;
    if (flags & FLAG_SYN) s += "SYN ";
    if (flags & FLAG_ACK) s += "ACK ";
    if (flags & FLAG_FIN) s += "FIN ";
    if (flags & FLAG_RST) s += "RST ";
    if (s.empty()) s = "NONE";
    else s.pop_back(); // remove trailing space
    return s;
}

inline void print_packet(const char* who, const TCPPacket& p) {
    printf("[%s] flags=%-12s seq=%-10u ack=%-10u",
           who, flags_to_str(p.flags).c_str(), p.seq, p.ack);
    if (p.data_len > 0)
        printf("  data=\"%.*s\"", (int)p.data_len, p.data);
    printf("\n");
}
