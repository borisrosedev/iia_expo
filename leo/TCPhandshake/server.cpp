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

static int make_udp_socket() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    return fd;
}

static void die(const char* msg) { perror(msg); exit(1); }

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
    srand((unsigned)time(nullptr));

    int fd = make_udp_socket();

    sockaddr_in local{};
    local.sin_family      = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port        = htons(SERVER_PORT);

    if (bind(fd, (sockaddr*)&local, sizeof(local)) < 0) die("bind");

    printf("=== SERVER listening on port %d ===\n\n", SERVER_PORT);

    // ── STEP 1: wait for SYN ─────────────────────────────────────────────────
    TCPPacket pkt{};
    sockaddr_in client{};
    socklen_t   clen = sizeof(client);

    ssize_t n = recvfrom(fd, &pkt, sizeof(pkt), 0,
                         (sockaddr*)&client, &clen);
    if (n < 0) die("recvfrom");
    pkt.ntoh();

    if (!(pkt.flags & FLAG_SYN)) {
        fprintf(stderr, "Expected SYN, got flags=%s\n",
                flags_to_str(pkt.flags).c_str());
        close(fd); return 1;
    }
    print_packet("SERVER <-- CLIENT", pkt);

    uint32_t client_isn = pkt.seq;   // client Initial Sequence Number
    uint32_t server_isn = rand();    // server picks its own ISN

    // ── STEP 2: send SYN-ACK ─────────────────────────────────────────────────
    TCPPacket synack{};
    synack.flags = FLAG_SYN | FLAG_ACK;
    synack.seq   = server_isn;
    synack.ack   = client_isn + 1;   // acknowledge client's SYN
    synack.hton();

    if (sendto(fd, &synack, sizeof(synack), 0,
               (sockaddr*)&client, clen) < 0) die("sendto SYN-ACK");
    synack.ntoh();
    print_packet("SERVER --> CLIENT", synack);

    // ── STEP 3: wait for ACK ─────────────────────────────────────────────────
    TCPPacket ack{};
    n = recvfrom(fd, &ack, sizeof(ack), 0,
                 (sockaddr*)&client, &clen);
    if (n < 0) die("recvfrom ACK");
    ack.ntoh();

    if (!(ack.flags & FLAG_ACK)) {
        fprintf(stderr, "Expected ACK, got flags=%s\n",
                flags_to_str(ack.flags).c_str());
        close(fd); return 1;
    }
    print_packet("SERVER <-- CLIENT", ack);

    // Validate: client must acknowledge server_isn+1
    if (ack.ack != server_isn + 1) {
        fprintf(stderr, "BAD ACK: expected %u, got %u\n",
                server_isn + 1, ack.ack);
        close(fd); return 1;
    }

    printf("\n=== Handshake COMPLETE — connection established ===\n");
    printf("    client ISN: %u\n", client_isn);
    printf("    server ISN: %u\n", server_isn);
    printf("    next expected from client: %u\n", ack.seq);

    close(fd);
    return 0;
}
