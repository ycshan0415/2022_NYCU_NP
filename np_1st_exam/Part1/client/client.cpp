#include <iostream>
#include <cstring>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <sstream>

using namespace std;

#define MAXLINE 1024

int cmd(char* str) {
    string s = "";
    for (int i = 0; i < strlen(str); ++i) {
        s += str[i];
        if (s == "get-file-list") {
            return 1;
        }
        else if (s == "get-file" && str[i+1] == ' ') {
            return 2;
        }
        else if (s == "exit")
            return 3;
    }
    return 0;
}

int NumOfFiles(char* str) {
    int count = 0;
    string name = "";
    // after get-file
    for (int i = 8; i < strlen(str) - 1; ++i) {
        if (str[i] == ' ') {
            if (name != "") {
                count++;
                name = "";
            }
        }
        else
            name += str[i];
    }
    if (name != "") {
        count++;
    } // last file
    return count;
}

int main(int argc, char*argv[]) {
    if (argc != 3) {
        cout << "IP PORT error\n";
        return 0;
    }
    in_addr_t IP = inet_addr(argv[1]);
    int port = atol(argv[2]);

    int udpfd;
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = IP;
    server_addr.sin_port = port;
    
    int enable = 1;
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        cout << "SO_REUSEADDR failed\n";

    int len;
    len = sizeof(server_addr);
    char send_buff[MAXLINE];
    char recv_buff[MAXLINE];

    cout << "UDP client is running\n";
    while(1) {
        memset(&recv_buff, 0, sizeof(recv_buff));
        memset(&send_buff, 0, sizeof(send_buff));

        cout << "% ";
        fgets(send_buff, MAXLINE, stdin);
        // input

        int send_num, recv_num;
        send_num = sendto(udpfd, send_buff, strlen(send_buff), 0, (struct sockaddr*)&server_addr, len);

        int type = cmd(send_buff);
        if (type == 1) {
            recv_num = recvfrom(udpfd,recv_buff, MAXLINE, 0, (struct sockaddr*)&server_addr, (socklen_t*)&len);
            cout << recv_buff << "\n";
        }
        else if (type == 2) {
            int n = NumOfFiles(send_buff);
            // cout << n;
            for (int t = 0; t < n; ++t) {
                //file name
                memset(&recv_buff, 0, sizeof(recv_buff));
                recv_num = recvfrom(udpfd, recv_buff, MAXLINE, 0, (struct sockaddr*)&server_addr, (socklen_t*)&len);
                string fname = "";
                for (int i = 0; i < strlen(recv_buff); ++i)
                    fname += recv_buff[i];

                // file data
                memset(&recv_buff, 0, sizeof(recv_buff));
                recv_num = recvfrom(udpfd, recv_buff, MAXLINE, 0, (struct sockaddr*)&server_addr, (socklen_t*)&len);

                ofstream f(fname, ios::out);
                for (int i = 0; i < strlen(recv_buff) - 1; ++i)
                    f << recv_buff[i];
                f.close();
            }
        }
        else if (type == 3) {
            break;
        }
    }

    return 0;
}