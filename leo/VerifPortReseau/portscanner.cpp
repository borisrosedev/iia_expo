/*
 * portscanner.cpp — Scanner de ports réseau (TCP + UDP)
 *
 * Utilise les appels système POSIX directement via les sockets BSD.
 * Multithreading avec std::thread, pas de framework externe.
 *
 * Compilation : g++ -std=c++17 -O2 -Wall -o portscanner portscanner.cpp -lpthread
 * Usage       : ./portscanner <hôte> [port_début] [port_fin] [--udp] [--threads N]
 */

#define _POSIX_C_SOURCE 200112L

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <stdexcept>
#include <cstring>
#include <cerrno>

/* POSIX / réseau */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* ------------------------------------------------------------------ */
/*  Constantes                                                          */
/* ------------------------------------------------------------------ */

static constexpr int DEFAULT_PORT_START  = 1;
static constexpr int DEFAULT_PORT_END    = 1024;
static constexpr int DEFAULT_THREADS     = 100;
static constexpr int MAX_THREADS         = 1024;
static constexpr int CONNECT_TIMEOUT_MS  = 500;
static constexpr int UDP_TIMEOUT_MS      = 800;

/* ------------------------------------------------------------------ */
/*  Table des services connus                                           */
/* ------------------------------------------------------------------ */

static const std::unordered_map<int, std::string> KNOWN_SERVICES = {
    {20,"FTP-data"},{21,"FTP"},{22,"SSH"},{23,"Telnet"},
    {25,"SMTP"},{53,"DNS"},{67,"DHCP"},{68,"DHCP"},
    {69,"TFTP"},{80,"HTTP"},{110,"POP3"},{119,"NNTP"},
    {123,"NTP"},{137,"NetBIOS"},{138,"NetBIOS"},{139,"NetBIOS"},
    {143,"IMAP"},{161,"SNMP"},{162,"SNMP-trap"},{194,"IRC"},
    {389,"LDAP"},{443,"HTTPS"},{445,"SMB"},{465,"SMTPS"},
    {514,"Syslog"},{515,"LPD"},{587,"SMTP-submission"},
    {631,"IPP"},{636,"LDAPS"},{993,"IMAPS"},{995,"POP3S"},
    {1080,"SOCKS"},{1194,"OpenVPN"},{1433,"MSSQL"},{1521,"Oracle"},
    {1723,"PPTP"},{2049,"NFS"},{2181,"Zookeeper"},{3000,"Dev-HTTP"},
    {3306,"MySQL"},{3389,"RDP"},{4369,"Erlang"},{5000,"UPnP"},
    {5432,"PostgreSQL"},{5672,"AMQP"},{5900,"VNC"},{5984,"CouchDB"},
    {6379,"Redis"},{6443,"Kubernetes"},{6881,"BitTorrent"},
    {8080,"HTTP-alt"},{8443,"HTTPS-alt"},{8888,"Jupyter"},
    {9200,"Elasticsearch"},{9300,"Elasticsearch"},{27017,"MongoDB"}
};

static const std::string& service_name(int port)
{
    static const std::string empty{};
    auto it = KNOWN_SERVICES.find(port);
    return (it != KNOWN_SERVICES.end()) ? it->second : empty;
}

/* ------------------------------------------------------------------ */
/*  RAII wrapper pour les descripteurs de fichiers                      */
/* ------------------------------------------------------------------ */

class FileDesc {
public:
    explicit FileDesc(int fd) : fd_(fd) {}
    ~FileDesc() { if (fd_ >= 0) ::close(fd_); }
    FileDesc(const FileDesc&)            = delete;
    FileDesc& operator=(const FileDesc&) = delete;
    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }
private:
    int fd_;
};

/* ------------------------------------------------------------------ */
/*  Utilitaires sockets                                                  */
/* ------------------------------------------------------------------ */

static bool set_nonblocking(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

/* ------------------------------------------------------------------ */
/*  Sonde TCP : retourne true si le port est ouvert                     */
/* ------------------------------------------------------------------ */

static bool tcp_probe(const sockaddr_in& base_addr, int port)
{
    FileDesc fd{ ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
    if (!fd.valid()) return false;
    if (!set_nonblocking(fd.get())) return false;

    sockaddr_in addr = base_addr;
    addr.sin_port    = htons(static_cast<uint16_t>(port));

    int rc = ::connect(fd.get(),
                       reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (rc == 0) return true;                  /* connexion immédiate */
    if (errno != EINPROGRESS) return false;    /* erreur dure         */

    /* Attente via select() */
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(fd.get(), &wset);

    timeval tv{};
    tv.tv_sec  = CONNECT_TIMEOUT_MS / 1000;
    tv.tv_usec = (CONNECT_TIMEOUT_MS % 1000) * 1000;

    if (::select(fd.get() + 1, nullptr, &wset, nullptr, &tv) <= 0)
        return false;

    int err = 0;
    socklen_t len = sizeof(err);
    ::getsockopt(fd.get(), SOL_SOCKET, SO_ERROR, &err, &len);
    return err == 0;
}

/* ------------------------------------------------------------------ */
/*  Sonde UDP : retourne true si ouvert ou filtré                       */
/*                                                                      */
/*  Logique : si on reçoit ICMP Port Unreachable → fermé.              */
/*            Si timeout → ouvert|filtré (comportement nmap -sU).      */
/* ------------------------------------------------------------------ */

static bool udp_probe(const sockaddr_in& base_addr, int port)
{
    FileDesc fd{ ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) };
    if (!fd.valid()) return false;

    /* Recevoir les erreurs ICMP sous Linux */
    int one = 1;
    ::setsockopt(fd.get(), SOL_IP, IP_RECVERR, &one, sizeof(one));

    sockaddr_in addr = base_addr;
    addr.sin_port    = htons(static_cast<uint16_t>(port));

    const char probe = '\x00';
    ::sendto(fd.get(), &probe, 1, 0,
             reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(fd.get(), &rset);

    timeval tv{};
    tv.tv_sec  = UDP_TIMEOUT_MS / 1000;
    tv.tv_usec = (UDP_TIMEOUT_MS % 1000) * 1000;

    if (::select(fd.get() + 1, &rset, nullptr, nullptr, &tv) > 0) {
        char     buf[256];
        sockaddr_in src{};
        socklen_t   slen = sizeof(src);
        ssize_t r = ::recvfrom(fd.get(), buf, sizeof(buf), MSG_ERRQUEUE,
                               reinterpret_cast<sockaddr*>(&src), &slen);
        if (r < 0 && errno == ECONNREFUSED)
            return false;   /* ICMP port unreachable → fermé */
    }

    return true;   /* timeout ou réponse → ouvert|filtré */
}

/* ------------------------------------------------------------------ */
/*  Contexte partagé entre tous les threads                             */
/* ------------------------------------------------------------------ */

struct ScanContext {
    sockaddr_in          addr{};
    int                  port_start{};
    int                  port_end{};
    bool                 scan_udp{};
    std::atomic<int>     next_port{};
    std::mutex           results_mutex;
    std::vector<int>     open_tcp;   /* ports TCP ouverts */
    std::vector<int>     open_udp;   /* ports UDP ouverts|filtrés */

    ScanContext(const sockaddr_in& a, int ps, int pe, bool udp)
        : addr(a), port_start(ps), port_end(pe), scan_udp(udp),
          next_port(ps) {}
};

/* ------------------------------------------------------------------ */
/*  Fonction de travail des threads                                     */
/* ------------------------------------------------------------------ */

static void scan_worker(ScanContext& ctx)
{
    while (true) {
        int port = ctx.next_port.fetch_add(1, std::memory_order_relaxed);
        if (port > ctx.port_end) break;

        bool tcp_open = tcp_probe(ctx.addr, port);
        bool udp_open = ctx.scan_udp && udp_probe(ctx.addr, port);

        if (tcp_open || udp_open) {
            std::lock_guard<std::mutex> lk(ctx.results_mutex);
            if (tcp_open) ctx.open_tcp.push_back(port);
            if (udp_open) ctx.open_udp.push_back(port);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Résolution DNS → sockaddr_in                                        */
/* ------------------------------------------------------------------ */

static sockaddr_in resolve_host(const std::string& host)
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    int rc = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (rc != 0)
        throw std::runtime_error(
            "Résolution échouée pour '" + host + "' : " +
            ::gai_strerror(rc));

    sockaddr_in addr = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
    ::freeaddrinfo(res);
    return addr;
}

/* ------------------------------------------------------------------ */
/*  Affichage des résultats                                             */
/* ------------------------------------------------------------------ */

static void print_results(const ScanContext& ctx, int port_start, int port_end)
{
    /* Fusion TCP + UDP dans un vecteur trié */
    struct Entry { int port; bool tcp; bool udp; };
    std::vector<Entry> rows;
    rows.reserve(ctx.open_tcp.size() + ctx.open_udp.size());

    /* Marquer TCP */
    for (int p : ctx.open_tcp)
        rows.push_back({p, true, false});

    /* Marquer UDP (fusionner si déjà présent) */
    for (int p : ctx.open_udp) {
        bool found = false;
        for (auto& e : rows)
            if (e.port == p) { e.udp = true; found = true; break; }
        if (!found)
            rows.push_back({p, false, true});
    }

    /* Tri par numéro de port */
    std::sort(rows.begin(), rows.end(),
              [](const Entry& a, const Entry& b){ return a.port < b.port; });

    std::cout << "\n"
              << std::left
              << std::setw(8)  << "PORT"
              << std::setw(10) << "PROTOCOLE"
              << std::setw(16) << "ÉTAT"
              << "SERVICE\n"
              << std::string(52, '-') << "\n";

    for (const auto& e : rows) {
        const std::string& svc = service_name(e.port);
        if (e.tcp)
            std::cout << std::left
                      << std::setw(8)  << e.port
                      << std::setw(10) << "TCP"
                      << std::setw(16) << "OUVERT"
                      << svc << "\n";
        if (e.udp)
            std::cout << std::left
                      << std::setw(8)  << e.port
                      << std::setw(10) << "UDP"
                      << std::setw(16) << "ouvert|filtré"
                      << svc << "\n";
    }

    if (rows.empty())
        std::cout << "Aucun port ouvert trouvé dans la plage "
                  << port_start << "–" << port_end << ".\n";
    else
        std::cout << "\n" << rows.size() << " port(s) trouvé(s).\n";
}

/* ------------------------------------------------------------------ */
/*  Aide                                                                */
/* ------------------------------------------------------------------ */

static void usage(const char* prog)
{
    std::cerr
        << "Usage: " << prog
        << " <hôte> [port_début] [port_fin] [--udp] [--threads N]\n\n"
        << "  hôte        Adresse IP ou nom d'hôte cible\n"
        << "  port_début  Premier port (défaut: " << DEFAULT_PORT_START << ")\n"
        << "  port_fin    Dernier port  (défaut: " << DEFAULT_PORT_END   << ")\n"
        << "  --udp       Activer le scan UDP en plus du TCP\n"
        << "  --threads N Threads simultanés (défaut: " << DEFAULT_THREADS
        << ", max: " << MAX_THREADS << ")\n";
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char* argv[])
{
    if (argc < 2) { usage(argv[0]); return EXIT_FAILURE; }

    std::string host       = argv[1];
    int         port_start = DEFAULT_PORT_START;
    int         port_end   = DEFAULT_PORT_END;
    int         num_threads = DEFAULT_THREADS;
    bool        scan_udp   = false;
    bool        ports_set  = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--udp") {
            scan_udp = true;
        } else if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
            if (num_threads < 1 || num_threads > MAX_THREADS) {
                std::cerr << "Threads invalide (1–" << MAX_THREADS << ")\n";
                return EXIT_FAILURE;
            }
        } else if (!ports_set) {
            port_start = std::stoi(arg);
            if (i + 1 < argc && argv[i+1][0] != '-')
                port_end = std::stoi(argv[++i]);
            else
                port_end = port_start;
            ports_set = true;
        }
    }

    if (port_start < 1 || port_end > 65535 || port_start > port_end) {
        std::cerr << "Plage de ports invalide (" << port_start
                  << "–" << port_end << ")\n";
        return EXIT_FAILURE;
    }

    /* Résolution DNS */
    sockaddr_in target_addr{};
    try {
        target_addr = resolve_host(host);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    char ip_str[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &target_addr.sin_addr, ip_str, sizeof(ip_str));

    std::cout << "Cible   : " << host << " (" << ip_str << ")\n"
              << "Ports   : " << port_start << " – " << port_end
              << "  (" << (port_end - port_start + 1) << " ports)\n"
              << "Threads : " << num_threads << "\n"
              << "UDP     : " << (scan_udp ? "oui" : "non") << "\n\n";

    /* Contexte partagé */
    ScanContext ctx(target_addr, port_start, port_end, scan_udp);

    int range          = port_end - port_start + 1;
    int actual_threads = std::min(num_threads, range);

    /* Lancement des threads */
    std::vector<std::thread> workers;
    workers.reserve(actual_threads);
    for (int i = 0; i < actual_threads; ++i)
        workers.emplace_back(scan_worker, std::ref(ctx));

    /* Affichage de la progression */
    while (true) {
        int done = ctx.next_port.load() - port_start;
        if (done >= range) break;
        std::cout << "\rProgression : " << done << "/" << range
                  << " ports scannés…" << std::flush;
        ::usleep(200'000);
    }

    for (auto& t : workers) t.join();

    /* Effacement de la ligne de progression */
    std::cout << "\r" << std::string(50, ' ') << "\r";

    /* Résultats */
    print_results(ctx, port_start, port_end);

    return EXIT_SUCCESS;
}
