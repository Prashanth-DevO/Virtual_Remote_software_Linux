#include "../include/utils.h"

std::string getLocalIP() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return "Error";

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(80);

    // Connect to external server (no data sent)
    inet_pton(AF_INET, "8.8.8.8", &serv.sin_addr);

    connect(sock, (sockaddr*)&serv, sizeof(serv));

    sockaddr_in name{};
    socklen_t namelen = sizeof(name);
    getsockname(sock, (sockaddr*)&name, &namelen);

    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer));

    close(sock);
    return std::string(buffer);
}