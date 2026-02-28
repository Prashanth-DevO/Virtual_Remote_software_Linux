#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main(){

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9002);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    char buffer[1024];
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);

    std::cout << "Discovery Service Running...\n";

    while(true){

        int n = recvfrom(sock, buffer, sizeof(buffer), 0,
                        (sockaddr*)&clientAddr, &len);

        if(n > 0){
            std::string msg(buffer, n);

            if(msg == "WHO_IS_CONTROLLER"){
                std::string reply = "CONTROLLER_AVAILABLE";

                sendto(sock, reply.c_str(), reply.size(), 0,
                      (sockaddr*)&clientAddr, len);
            }
        }
    }
}
