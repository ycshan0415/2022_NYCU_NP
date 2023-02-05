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
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>

using namespace std;

#define SA struct sockaddr
#define SERV_PORT 8010
#define MAXLINE 1024
#define PATHNAME "."
#define PROJ_ID 0x6666

struct info {
    bool logio;
    char uname[100];
    int chance;
    bool inGame;
    char ans[4];
};

struct users {
    // in shared mem
    char udata[2048];
    char emails[1024];
};

key_t key = ftok("sharedMem", 66);
int shmid = shmget(key, sizeof(users), 0666|IPC_CREAT);

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

string regist(vector<string> cmd) {
    string msg;
    int size = cmd.size();
    if (size != 4) {
        msg = "Usage: register <username> <email> <password>";
        return msg;
    }

    string name = cmd[1];
    string email = cmd[2];
    string password = cmd[3];
    bool registered = false;
    users* shm = (users* )shmat(shmid, NULL, 0);
    char* account = shm->udata;
    char* em = shm->emails;
    for (int i = 0; i < strlen(account); ++i) {
        int t = 0;
        int j;
        if (account[i] == name[0]) {
            for (j = i; j < (i + name.size()); ++j, ++t) {
                if (account[j] == name[t])
                    registered = true;
                else {
                    registered = false;
                    break;
                }
            }
        }
        if (registered && account[j] == '&') {
            msg = "Username is already used.";
            shmdt(shm);
            return msg;
        }
    }
    registered = false;
    for (int i = 0; i < strlen(em); ++i) {
        int t = 0;
        int j;
        if (em[i] == email[0]) {
            for (j = i; j < (i + email.size()); ++j) {
                if (em[j] == email[t])
                    registered = true;
                else {
                    registered = false;
                    break;
                }
                t++;
            }
        }
        if (registered && em[j] == '/') {
            msg = "Email is already used.";
            shmdt(shm);
            return msg;
        }
    }
    if (!registered) {
        int totlen, curlen;
        // username + password
        totlen = strlen(account);
        curlen = name.size(); 

        for (int i = totlen, j = 0; i < totlen + curlen; ++i, ++j)
            account[i] = name[j];
        account[totlen + curlen] = '&';

        totlen += curlen;
        totlen++;
        curlen = password.size();
        for (int i = totlen, j = 0; i < totlen + curlen; ++i, ++j)
            account[i] = password[j];
        account[totlen + curlen] = '/';

        // email
        totlen = strlen(em);
        curlen = email.size(); 
        for (int i = totlen, j = 0; i < totlen + curlen; ++i, ++j)
            em[i] = email[j];
        em[totlen + curlen] = '/';
        
        msg = "Register successfully.";
        shmdt(shm);

    }
    
    return msg;
}

string login(vector<string> cmd, info* ptr) {
    string msg;
    int size = cmd.size();
    if (size != 3) {
        msg = "Usage: login <username> <password>";
        return msg;
    }

    string name = cmd[1];
    string password = cmd[2];
    bool found = false;

    users* shm = (users* )shmat(shmid, NULL, 0);
    char* account = shm->udata;
    if (ptr->logio)
        msg = "Please logout first.";
    else {
        int idx;
        for (int i = 0; i < strlen(account); ++i) {
            int t = 0;
            int j;
            if (account[i] == name[0]) {
                for (j = i; j < (i + name.size()); ++j) {
                    if (account[j] == name[t])
                        found = true;
                    else {
                        found = false;
                        break;
                    }
                    t++;
                }
            }
            if (found && account[j] == '&') {
                idx = j + 1;
                break;
            }
        }
        if (found) {
            int t = 0;
            for (int i = idx; i < (idx + password.size()); ++i) {
                if (account[i] != password[t]) {
                    msg = "Password not correct.";
                    shmdt(shm);
                    return msg;                    
                }
                t++;

            }
            
            msg = "Welcome, ";
            msg += name;
            msg += ".";
            ptr->logio = true;
            strcpy(ptr->uname, name.c_str());
        }
        else if (!found)
            msg = "Username not found.";
    }

    return msg;
}

string logout(vector<string> cmd, info* ptr) {
    string msg;
    int size = cmd.size();
    if (size != 1) {
        msg = "Usage: logout\n";
        return msg;
    }
    if (ptr->logio) {
        msg = "Bye, ";
        msg += ptr->uname;
        msg += ".";
        ptr->logio = false;
    }
    else
        msg = "Please login first.\n";

    return msg;
}

string rule(vector<string> cmd) {
    string msg;
    int size = cmd.size();
    if (size != 1) {
        msg = "game-rule\n";
        return msg;
    }

    msg = "1. Each question is a 4-digit secret number.\n\
2. After each guess, you will get a hint with the following information:\n\
2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n\
2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\n\
The hint will be formatted as \"xAyB\".\n\
3. 5 chances for each question.\n";

    return msg;
}

string game(vector<string> cmd, info* ptr) {
    string msg = "";

    if (!ptr->inGame) {
        int size = cmd.size();
        if (size > 2) {
            msg = "Usage: start-game <4-digit number>";
            return msg;
        }
        else if (!ptr->logio) {
            msg = "Please login first.";
            return msg;
        }
        else if (size == 2) {
            if (cmd[1].size() != 4 || !isNumber(cmd[1])) {
                msg = "Usage: start-game <4-digit number>";
                return msg;
            }
            else {
                for (int i = 0; i < 4; ++i)
                    ptr->ans[i] = cmd[1][i];
                msg = "Please typing a 4-digit number:";
                ptr->chance = 1;
                ptr->inGame = true;
                return msg;
            }
        }
        else {
            bool used[10] = {false};
            for (int i = 0; i < 4; ++i) {
                int num;
                srand(time(NULL));
                do {
                    num = rand() % 10;
                } while (used[num - 1]);
                ptr->ans[i] = num + '0';
                used[num - 1] = true;
            }
            msg = "Please typing a 4-digit number:";
            ptr->chance = 1;
            ptr->inGame = true;
            return msg;
        }
    }
    else if (ptr->inGame) {
        string guess;
        guess = cmd[0];
        
        int size = guess.length();
        if (size != 4) {
            msg = "Your guess should be a 4-digit number.";
            return msg;
        }
        bool pos[4] = {false};
        int cntA = 0, cntB = 0;
        for (int i = 0; i < 4; ++i) { // A
            if (guess[i] == ptr->ans[i]) {
                cntA++;
                pos[i] = true;
                guess[i] = ' ';
            }
        }
        for (int i = 0; i < 4; ++i) { // B
            for (int j = 0; j < 4; ++j) { // ans
                if (j == i || pos[j]) continue;
                if (guess[i] == ptr->ans[j]){
                    cntB++;
                    pos[j] = true;
                    break;
                }
            }
        }

        if (cntA == 4) {
            msg = "You got the answer!";
            ptr->inGame = false;
            ptr->chance = 0;
            return msg;
        }
        else if (ptr->chance == 5) {
            msg = "You lose the game!";
            ptr->inGame = false;
            ptr->chance = 0;
            return msg;
        }
        else {
            msg += cntA + '0';
            msg += "A";
            msg += cntB + '0';
            msg += "B";
            (ptr->chance)++;
            return msg;
        }
    }

    return msg;

}

vector<string> tokenize(char* buffer){
    stringstream ss(buffer);
    vector<string> cmd;
    string token;
    while(getline(ss, token, ' ')){
        if(token.empty())
            continue;
        else 
            cmd.push_back(token);
    }

    return cmd;
}

void TCPcmd(vector<string> cmd, string &msg, info* ptr) {
    if (cmd[0] == "exit") { // TCP
        cout << "tcp get msg: exit";
        msg = "";
    }
    else if (ptr->inGame){ // TCP
        msg = game(cmd, ptr);
    }
    else if (cmd[0] == "login") { // TCP
        msg = login(cmd, ptr);
    }
    else if (cmd[0] == "logout") { // TCP
        msg = logout(cmd, ptr);
    }
    else if (cmd[0] == "start-game") { // TCP
        msg = game(cmd, ptr);
    }
    else
        msg = "";
}

void UDPcmd(vector<string> cmd, string &msg) {
    if (cmd[0] == "register") { // UDP
        msg = regist(cmd);
        
    }
    else if (cmd[0] == "game-rule") { // UDP
        msg = rule(cmd);
    }
    else
        msg = "";
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

    // UDP socket
    int udpfd;
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1)
        std::printf("UDP server failed");
    
    struct sockaddr_in server, client;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    
    // port reuse
    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::printf("SO_REUSEADDR failed");
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::printf("SO_REUSEADDR failed");
    
    // bind (TCP)
    if (bind(listenfd, (SA*)&server, sizeof(server)) != 0)
        std::printf("TCP bind failed");
    // listen (TCP)
    if (listen(listenfd, 10) != 0)
        std::printf("TCP listen failed");
    // bind (UDP)
    if (bind(udpfd, (SA*)&server, sizeof(server)) != 0)
        std::printf("UDP bind failed");

    // clear the descriptor set
    fd_set rset;
    FD_ZERO(&rset);
    // get maxfd
    int maxfdp1;
    maxfdp1 = max(listenfd, udpfd) + 1;

    socklen_t len;
    int connfd, nready;
    char buffer[MAXLINE];
    const char* msg;
    ssize_t n;  

    cout << "UDP server is running\n";
    cout << "TCP server is running\n";
    while(1) {
        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        // select the ready descriptor
        nready = select(maxfdp1, &rset, NULL, NULL, NULL);

        // if tcp socket is readable then handle
        // it by accepting the connection
        pid_t childpid;
        if (FD_ISSET(listenfd, &rset)) {
            string msg;
            len = sizeof(client);
            connfd = accept(listenfd, (SA*)&client, &len);
            cout << "New connection.\n";
            childpid = fork();
            if (childpid == 0) {
                info* ptr = new info;
                ptr->logio = false;
                ptr->chance = 0;
                ptr->inGame = false;
                close(listenfd);
                while(1) {
                    bzero(buffer, sizeof(buffer));
                    recv(connfd, buffer, sizeof(buffer), 0);
                    vector<string> cmd = tokenize(buffer);
                    TCPcmd(cmd, msg, ptr);
                    const char* response = msg.c_str();
                    send(connfd, response, sizeof(buffer), 0);
                    
                    shmdt(ptr);
                }
                close(connfd);
                std::exit(0);
            }
            else // parent 
                close(connfd);
        }
        // if udp socket is readable receive the msg.
        if (FD_ISSET(udpfd, &rset)) {
            string msg;
            len = sizeof(client);
            bzero(buffer, sizeof(buffer));
            n = recvfrom(udpfd, buffer, sizeof(buffer), 0, (SA*)&client, &len);
            vector<string> cmd = tokenize(buffer);
            UDPcmd(cmd, msg);
            const char* response = msg.c_str();
            sendto(udpfd, response, sizeof(buffer), 0, (SA*)&client, sizeof(client));
        }
    }

    shmctl(shmid, IPC_RMID, 0); // won't reach here

    return 0;
}