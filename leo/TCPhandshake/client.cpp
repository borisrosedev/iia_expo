#include "common.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ─── helpers ─────────────────────────────────────────────────────────────────

static void die(const char* msg) { perror(msg); exit(1); }

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const char* server_ip = (argc >= 2) ? argv[1] : "127.0.0.1";

    srand((unsigned)time(nullptr));

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) die("socket");

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", server_ip);
        return 1;
    }

    printf("=== CLIENT connecting to %s:%d ===\n\n", server_ip, SERVER_PORT);

    uint32_t client_isn = rand();   // client Initial Sequence Number

    // ── STEP 1: send SYN ─────────────────────────────────────────────────────
    TCPPacket syn{};
    syn.flags = FLAG_SYN;
    syn.seq   = client_isn;
    syn.ack   = 0;
    print_packet("CLIENT --> SERVER", syn);
    syn.hton();

    if (sendto(fd, &syn, sizeof(syn), 0,
               (sockaddr*)&server, sizeof(server)) < 0) die("sendto SYN");

    // ── STEP 2: receive SYN-ACK ──────────────────────────────────────────────
    TCPPacket synack{};
    sockaddr_in from{};
    socklen_t   flen = sizeof(from);

    ssize_t n = recvfrom(fd, &synack, sizeof(synack), 0,
                         (sockaddr*)&from, &flen);
    if (n < 0) die("recvfrom SYN-ACK");
    synack.ntoh();

    if ((synack.flags & (FLAG_SYN | FLAG_ACK)) != (FLAG_SYN | FLAG_ACK)) {
        fprintf(stderr, "Expected SYN-ACK, got flags=%s\n",
                flags_to_str(synack.flags).c_str());
        close(fd); return 1;
    }
    print_packet("CLIENT <-- SERVER", synack);

    // Validate: server must acknowledge our SYN (client_isn + 1)
    if (synack.ack != client_isn + 1) {
        fprintf(stderr, "BAD SYN-ACK: expected ack=%u, got %u\n",
                client_isn + 1, synack.ack);
        close(fd); return 1;
    }

    uint32_t server_isn = synack.seq;

    // ── STEP 3: send ACK ─────────────────────────────────────────────────────
    TCPPacket ack{};
    ack.flags = FLAG_ACK;
    ack.seq   = client_isn + 1;       // our sequence advances past the SYN
    ack.ack   = server_isn + 1;       // acknowledge server's SYN
    print_packet("CLIENT --> SERVER", ack);
    ack.hton();

    if (sendto(fd, &ack, sizeof(ack), 0,
               (sockaddr*)&server, sizeof(server)) < 0) die("sendto ACK");

    printf("\n=== Handshake COMPLETE — connection established ===\n");
    printf("    client ISN: %u\n", client_isn);
    printf("    server ISN: %u\n", server_isn);

    close(fd);
    return 0;
}
