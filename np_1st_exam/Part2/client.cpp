#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

using namespace std;

#define MAXLINE 1024

int main(int argc, char*argv[]) {
    if (argc != 3) {
        cout << "IP PORT error\n";
        return 0;
    }
    in_addr_t IP = inet_addr(argv[1]);
    int port = atol(argv[2]);

    int tcpfd;
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = IP;
    server_addr.sin_port = port;

    int enable = 1;
    if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        cout << "SO_REUSEADDR failed\n";

    socklen_t len;
    len = sizeof(server_addr);
    char send_buff[MAXLINE], recv_buff[MAXLINE];

    if (connect(tcpfd, (struct sockaddr*)&server_addr, len) < 0) {
        cout << "Connect error\n";
        return 0;
    }

    while(1) {
        memset(&send_buff, 0, sizeof(send_buff));
        memset(&recv_buff, 0, sizeof(recv_buff));
        
        cout << "% ";
        fgets(send_buff, MAXLINE, stdin);

        int send_num, recv_num;
        send_num = send(tcpfd, send_buff, strlen(send_buff), 0);
        // cout << strlen(send_buff) << '\n';

        recv_num = recv(tcpfd, recv_buff, MAXLINE, 0);
        cout << recv_buff << "\n";
        
        string s = "";
        bool leave = false;
        for (int i = 0; i < 4; ++i) {
            s+= send_buff[i];
            if (s == "exit") {
                leave = true;
                break;
            }
        }
        if (leave)
            break;
    }

    return 0;
}