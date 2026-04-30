#include "../include/udp_receiver.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>


bool UdpReceiver::start(int port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return false;

    int flags = fcntl(sockfd, F_GETFL,0);
    fcntl(sockfd , F_SETFL , flags | O_NONBLOCK); //for stopping Blocking nature

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
    return true;
}

int UdpReceiver::receive(void* buffer, int size, sockaddr_in* outSender) {
    socklen_t len = sizeof(sockaddr_in);
    std::memset(outSender, 0, sizeof(sockaddr_in));
    return recvfrom(sockfd, buffer, size, 0, (sockaddr*)outSender, &len);
}

void UdpReceiver::stop(){
    if(sockfd!=-1){
        close(sockfd);
        sockfd=-1;
    }
}