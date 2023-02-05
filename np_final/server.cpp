#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <bits/stdc++.h> 

using namespace std;

#define SA struct sockaddr
#define PORT 8888
#define MAXLINE 4096
#define MAXCLIENT 5

struct info {
    bool welcome = false;
    bool mode = 1; // 0 for mute, 1 for open
};

map<int, info> cinfo;

int max(int x, int y)
{
	return x > y ? x : y;
}

bool isNumber(const string& s)
{
    for (char const &ch : s) {
        if (isdigit(ch) == 0)
            return false;
    }
    return true;
}

string welcome(int id) {
    string msg = "Welcome, user";
    msg += to_string(id);
    msg += ".\n";

    return msg;
}

void exitt(int id) {
    cinfo[id].welcome = false;
    cinfo[id].mode = 1;
}

string mute(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 1) {
        msg = "invalid input\n";
        return msg;
    }

    if (cinfo[id].mode == 0) {
        msg = "You are already in mute mode.\n";
    }
    else if (cinfo[id].mode == 1) {
        cinfo[id].mode = 0;
        msg = "Mute mode.\n";
    }
    
    return msg;
}

string unmute(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 1) {
        msg = "invalid input\n";
        return msg;
    }

    if (cinfo[id].mode == 0) {
        cinfo[id].mode = 1;
        msg = "Unmute mode.\n";
    }
    else if (cinfo[id].mode == 1) {
        msg = "You are already in unmute mode.\n";
    }
    
    return msg;
}

string yell(char* buffer, int id) {
    string msg = "user";
    msg += to_string(id);
    msg += ": ";
    for (int i = 5; i < strlen(buffer); i++) {
        msg += buffer[i];
    }
    msg += "\n";

    return msg;
}

string tell(char* buffer, int id) {
    string msg = "user";
    msg += to_string(id);
    msg += " told you: ";
    for (int i = 11; i < strlen(buffer); i++) {
        msg += buffer[i];
    }
    msg += "\n";

    return msg;
}

vector<string> tokenize(char* buffer){
    vector<string> cmd;

    string tmp = "";
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == ' ') {
            cmd.push_back(tmp);
            tmp = "";
            continue;
        }
        if (buffer[i] == '\n') {
            break;
        }
        tmp += buffer[i];
    }
    cmd.push_back(tmp);

    return cmd;
}

// TCP: socket->bind->listen->accept->do something->close
// UDP: socket->bind->receive->close
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "PORT error";
        return 0;
    }
    int port = atoi(argv[1]);

	// TCP socket
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (listenfd == -1)
        std::printf("TCP server failed");

    
    struct sockaddr_in server, client;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    
    // port reuse
    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::printf("SO_REUSEADDR failed");
    
    // bind (TCP)
    if (bind(listenfd, (SA*)&server, sizeof(server)) != 0)
        std::printf("TCP bind failed");
    // listen (TCP)
    if (listen(listenfd, 10) != 0)
        std::printf("TCP listen failed");

    int client_socket[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++)
	    client_socket[i] = 0;

    // clear the descriptor set
    fd_set readfds;
    FD_ZERO(&readfds);

    socklen_t len = sizeof(server);
    int connfd, newfd, nready;
    char buffer[MAXLINE];
    string msg;
    ssize_t n;  

    // cout << "UDP server is running\n";
    // cout << "TCP server is running\n";
    while(1) {
        // set listenfd and udpfd in readset
        FD_SET(listenfd, &readfds);
        // get maxfd
        int maxfd;
        maxfd = max(listenfd, 0);

        for (int i = 0 ; i < MAXCLIENT; i++) {  
            connfd = client_socket[i];  
            //if valid socket descriptor then add to read list 
            if(connfd > 0)  
                FD_SET(connfd , &readfds);  
            //highest file descriptor number, need it for the select function 
            if(connfd > maxfd)  
                maxfd = connfd;  
        }  

        // select the ready descriptor
        nready = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        // if tcp socket is readable then handle
        // it by accepting the connection
        if (FD_ISSET(listenfd, &readfds)) {
            string msg;
            len = sizeof(client);
            newfd = accept(listenfd, (SA*)&client, &len);
            // cout << "New connection.\n";
            //add new socket to array of sockets 
            for (int i = 0; i < MAXCLIENT; i++) {  
                //if position is empty 
                if (client_socket[i] == 0 ) {  
                    client_socket[i] = newfd; 
                    cout << i << " connect\n"; 
                    // welcome
                    msg = welcome(i);
                    const char* response = msg.c_str();
                    send(client_socket[i], response, strlen(response), 0);

                    break;  
                }  
            } 
        }

        for (int i = 0; i < MAXCLIENT; i++) {
            connfd = client_socket[i];
            int id = i;
            if (FD_ISSET(connfd , &readfds)) {
                bzero(buffer, sizeof(buffer));
                int valread;
                if ((valread = recv(connfd, buffer, sizeof(buffer), 0)) == 0) { // receive
                    // Close the socket and mark as 0 in list for reuse
                    exitt(id);
                    // close(connfd);  // //?
                    client_socket[i] = 0;  
                }
              
                //Echo back the message that came in 
                else {
                    // cout << buffer << "@\n";

                    vector<string> cmd = tokenize(buffer);
                    if (cmd[0] == "mute") { // mute
                        msg = mute(cmd, id);
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);
                    }
                    else if (cmd[0] == "unmute") { // unmute
                        msg = unmute(cmd, id);
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);
                    }
                    else if (cmd[0] == "yell") { // yell
                        msg = yell(buffer, id);                        
                        const char* response = msg.c_str();
                        // send(connfd, response, strlen(response), 0);
                        
                        for (int j = 0; j < MAXCLIENT; j++) {
                            if (id == j) // self
                                continue;
                            int tempfd = client_socket[j];
                            if (cinfo[j].mode == 1)
                                send(tempfd, response, strlen(response), 0);
                        }
                    }
                    else if (cmd[0] == "tell") {
                        msg = tell(buffer, id);
                        const char* response = msg.c_str();
                        int uid = int(buffer[9] - '0');
                        
                        if (uid != id) {
                            int tempfd = client_socket[uid];

                            if (tempfd) {
                                if (cinfo[uid].mode)
                                    send(tempfd, response, strlen(response), 0);
                            }
                            else {
                                msg = "user";
                                msg += buffer[9];
                                msg += " does not exist.\n";
                                const char* response = msg.c_str();
                                send(connfd, response, strlen(response), 0);
                            }
                        }
                        else {
                            msg = "Any client can not send messages to himself\n";
                            const char* response = msg.c_str();
                            send(connfd, response, strlen(response), 0);
                        }
                    }
                    else if (cmd[0] == "exit") {
                        exitt(id);
                        cout << id << " exit\n";
                        // close(connfd);  // //?
                        client_socket[i] = 0; 
                    }
                    // cout << msg << '\n';
                    // cout << response << '\n';
                } 
            }
        }
        
    }
    // end program

    return 0;
}

