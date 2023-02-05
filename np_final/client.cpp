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
#include <thread>

using namespace std;

#define SA struct sockaddr
#define PORT 8888
#define MAXLINE 4096

void send_thread(char* buffer, int sockfd) {
    while (1) {
        memset(buffer, 0, sizeof(buffer));
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

        const char *c = line.c_str();
        // cout << c << endl;
        //send recv
        if (cmd[0] == "yell" || cmd[0] == "tell")
            send(sockfd, c, strlen(c), 0);
        else if (cmd[0] == "mute" || cmd[0] == "unmute") {
            send(sockfd, c, strlen(c), 0);
        }
        else if (cmd[0] == "exit") {
            send(sockfd, c, strlen(c), 0);
            close(sockfd);
            exit(1);
            return;
        }
        else {
            cout << "not a command\n";
        }
    }
}

void recv_thread(char* buffer, int sockfd) {
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_num = recv(sockfd, buffer, sizeof(buffer), 0);
        buffer[recv_num] = '\0';
        cout << buffer;
    }
}

// TCP: socket->connect->send->receive->close
// UDP: socket->send->close
int main(int argc, char* argv[]) {       
    if (argc != 3) {
        cout << "IP PORT error\n";
        return 0;
    }
    int port = atoi(argv[2]);

    // socket
    int tcpfd;
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;  
    bzero(&server, sizeof(server));
    //memset(&server, 0, sizeof(server));

	// assign IP, PORT
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_port = htons(port);
    int enable = 1;
    if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        printf("SO_REUSEADDR failed\n");

    // timeout
    // struct timeval tv;
    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(tcpfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    char buffer1[MAXLINE];
    char buffer2[MAXLINE];
    int n;
    socklen_t len = sizeof(server);
    //connect
    if (connect(tcpfd, (SA*)&server, sizeof(server)) != 0) {
        printf("client connect failed\n");
        return 0;
    }
    else {
        cout << "**************************\n";
        cout << "*Welcome to the BBS server.*\n";
        cout << "**************************\n";
        // recv(tcpfd, buffer1, sizeof(buffer1), 0);
        // cout << buffer1;
    }

    while (1) {
        thread se(send_thread, buffer1, tcpfd);
        thread re(recv_thread, buffer2, tcpfd);

        se.join();
        re.join();

        // memset(buffer, 0, sizeof(buffer));
        // // if (recv(tcpfd, buffer, sizeof(buffer), 0))
        // //     cout << buffer;

        // string line;
        // getline(cin, line);
        // stringstream ss(line);

        // vector<string> cmd;
        // string token;
        // while(getline(ss, token, ' ')){
        //     if(token.empty())
        //         continue;
        //     else 
        //         cmd.push_back(token);
        // }
        
        // const char *c = line.c_str();
        // // cout << c << endl;
        // //send recv
        // if (cmd[0] == "yell" || cmd[0] == "tell")
        //     send(tcpfd, c, sizeof(buffer), 0);
        // else if (cmd[0] == "mute" || cmd[0] == "unmute") {
        //     send(tcpfd, c, sizeof(buffer), 0);
        //     recv(tcpfd, buffer, sizeof(buffer), 0);
        //     cout << buffer;
        // }
        // else if (cmd[0] == "exit") {
        //     send(tcpfd, c, sizeof(buffer), 0);
        //     break;
        // }
        // else {
        //     recv(tcpfd, buffer, sizeof(buffer), 0);
        //     cout << buffer;
        // }
    }

    close(tcpfd);
    std::exit(0);

    return 0;
}