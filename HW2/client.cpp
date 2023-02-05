#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace std;

#define SA struct sockaddr
#define PORT 8888
#define MAXLINE 1024

bool isNumber(const string& s)
{
    for (char const &ch : s) {
        if (isdigit(ch) == 0)
            return false;
    }
    return true;
}

// TCP: socket->connect->send->receive->close
// UDP: socket->send->close
int main(int argc, char* argv[]) {       
    // if (argc != 3) {
    //     cout << "IP PORT error\n";
    //     return 0;
    // }
    // int port = atoi(argv[2]);

    // socket
    int tcpfd, udpfd;
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    udpfd = socket(AF_INET, SOCK_DGRAM, 0); 

    struct sockaddr_in server;  
    bzero(&server, sizeof(server));
    //memset(&server, 0, sizeof(server));

	// assign IP, PORT
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(PORT);
    int enable = 1;
    if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        printf("SO_REUSEADDR failed\n");
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        printf("SO_REUSEADDR failed\n");

    char buffer[MAXLINE];
    int n;
    socklen_t len = sizeof(server);
    //connect
    if (connect(tcpfd, (SA*)&server, sizeof(server)) != 0) {
        printf("client connect failed\n");
        return 0;
    }
    // else
    //     cout << "*****Welcome to Game 1A2B*****\n";
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int type; // TCP = 1, UDP = 0
        string line;
        getline(cin, line);
        stringstream ss(line);
        vector<string> cmd;
        string token;
        while(getline(ss, token, ' ')){
            if(token.empty())
                continue;
            else 
                cmd.push_back(token);
        }
        if (cmd[0] == "register" || (cmd[0] == "list" && cmd[1] == "rooms") || (cmd[0] == "list" && cmd[1] == "users")) // UDP
            type = 0;
        else if (cmd[0] == "login" || cmd[0] == "logout" || cmd[0] == "create" || cmd[0] == "join" || cmd[0] == "invite" || (cmd[0] == "list" && cmd[1] == "invitations") 
                || cmd[0] == "leave" || cmd[0] == "start" || cmd[0] == "guess") { // TCP
            type = 1;
            // cout << cmd[0];
        }
        else 
            type = 2;
        // else
        //     cout << "invalid input\n";
        
        const char *c = line.c_str();

        if (type == 1) { // TCP
            //send recv
            send(tcpfd, c, sizeof(buffer), 0);
            recv(tcpfd, buffer, sizeof(buffer), 0);
        }
        else if (type == 0) { // UDP
            // send
            sendto(udpfd, c, strlen(c), 0, (const SA*)&server, sizeof(server));
            // receive 
            n = recvfrom(udpfd, (char*)buffer, MAXLINE, 0, (SA*)&server, &len);
            buffer[n] = '\0'; 
        }
       
        cout << buffer;
        if (cmd[0] == "exit") // TCP
            return 0;;
    }

    close(tcpfd);
    close(udpfd);
    std::exit(0);

    return 0;
}