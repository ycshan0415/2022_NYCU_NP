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
#include <set>

using namespace std;

#define SA struct sockaddr
#define PORT 8888
#define MAXLINE 1024
#define PATHNAME "."
#define PROJ_ID 0x6666
#define MAXCLIENT 100

struct invit {
    string invitier;
    string email;
    string room;
    string code;
};

struct info {
    bool logio = false;
    char uname[100];
    bool manager = false;
    bool inRoom = false;
    string room = "";
    vector<invit> invitations;
    
};

bool cmp(const invit &a, const invit &b)
{
    return a.room < b.room;
}

map<string, string> upw;
map<string, string> uem;
map<string, bool> ulog;
map<int, info> cinfo;
map<string, int> roomID; // 0 for public, 1 for private
map<string, string> roomCode; 
map<string, int> roomStatus; // 0 for waiting, 1 for started
map<string, vector<int>> roomMember;
map<string, string> roomAns;
map<string, int> gameRounds;
map<string, int> roomIter;
map<string, int> gameGuess;

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
        msg = "invalid input\n";
        return msg;
    }

    string name = cmd[1];
    string email = cmd[2];
    string password = cmd[3];

    for (auto it = upw.begin(); it != upw.end(); it++) {
        if (it->first == name) {
            msg = "Username or Email is already used\n";
            return msg;
        }            
    }
    for (auto it = uem.begin(); it != uem.end(); it++) {
        if (it->second == email) {
            msg = "Username or Email is already used\n";
            return msg;
        }            
    }

    upw[name] = password;
    uem[name] =  email;
    ulog[name] = false;

    msg = "Register Successfully\n";
    return msg;
}

string login(vector<string> cmd, int id) {
    string msg = "none";
    int size = cmd.size();
    if (size != 3) {
        msg = "invalid input\n";
        return msg;
    }

    string name = cmd[1];
    string password = cmd[2];
    
    bool found = false;
    for (auto it = upw.begin(); it != upw.end(); it++) {
        if (it->first == name) {
            found = true;
        }            
    }
    if (!found) {
        msg = "Username does not exist\n";
    }
    else if (cinfo[id].logio) {
        msg = "You already logged in as ";
        msg += cinfo[id].uname;
        msg += "\n";
    }
    else if (ulog[name]) {
        msg = "Someone already logged in as ";
        msg += name;
         msg += "\n";
    }
    else if (upw[name] != password) {
        msg = "Wrong password\n";
    }
    else {
        cinfo[id].logio = true;
        strcpy(cinfo[id].uname, name.c_str());
        ulog[name] = true;
        msg = "Welcome, ";
        msg += name;
        msg += "\n";
    }

    return msg;
}

string logout(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 1) {
        msg = "invalid input\n";
        return msg;
    }

    if (cinfo[id].logio && !cinfo[id].inRoom) {
        ulog[cinfo[id].uname] = false;
        cinfo[id].logio = false;
        msg = "Goodbye, ";
        msg += cinfo[id].uname;
        msg += "\n";
    }
    else if (cinfo[id].logio && cinfo[id].inRoom) {
        msg = "You are already in game room ";
        msg += cinfo[id].room;
        msg += ", please leave game room\n";
    }   
    else
        msg = "You are not logged in\n";

    return msg;
}

string createPublic(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 4) {
        msg = "invalid input\n";
        return msg;
    }

    string RID = cmd[3];
    if (!isNumber(RID)) {
        msg = "invalid input\n";
        return msg;
    }

    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].inRoom) {
        msg = "You are already in game room ";
        msg += cinfo[id].room;
        msg += ", please leave game room\n";
    }
    else {
        bool reused = false;
        for (auto it = roomID.begin(); it != roomID.end(); it++) {
            if (it->first == RID) {
                reused = true;
                break;
            }
        }
        if (reused) {
            msg = "Game room ID is used, choose another one\n";
            return msg;
        }
        else {
            cinfo[id].manager = true;
            cinfo[id].inRoom = true;
            cinfo[id].room = RID;
            roomID[RID] = 0;
            roomStatus[RID] = 0;
            roomMember[RID].push_back(id);
            msg = "You create public game room ";
            msg += RID;
            msg += "\n";
        }
    }

    return msg;
}

string createPrivate(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 5) {
        msg = "invalid input\n";
        return msg;
    }

    string RID = cmd[3];
    string code = cmd[4];
    if (!isNumber(RID)) {
        msg = "invalid input\n";
        return msg;
    }

    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].inRoom) {
        msg = "You are already in game room ";
        msg += cinfo[id].room;
        msg += ", please leave game room\n";
    }
    else {
        bool reused = false;
        for (auto it = roomID.begin(); it != roomID.end(); it++) {
            if (it->first == RID) {
                reused = true;
                break;
            }
        }
        if (reused) {
            msg = "Game room ID is used, choose another one\n";
            return msg;
        }
        else {
            cinfo[id].manager = true;
            cinfo[id].inRoom = true;
            cinfo[id].room = RID;
            roomID[RID] = 1;
            roomCode[RID] = code;
            roomStatus[RID] = 0;
            roomMember[RID].push_back(id);
            msg = "You create private game room ";
            msg += RID;
            msg += "\n";
        }
    }

    return msg;
}

string listRoom(vector<string> cmd) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    msg = "List Game Rooms";
    vector<int> IDs;
    for (auto it = roomID.begin(); it != roomID.end(); it++) {
        IDs.push_back(stoi(it->first));
    }
    if (IDs.empty()) {
        msg += "\nNo Rooms\n";
        return msg;
    }

    sort(IDs.begin(), IDs.end()); // ascending
    for (int i = 0; i < IDs.size(); i++) {
        msg += "\n";
        msg += to_string(i + 1);
        msg += ". ";
        for (auto it = roomID.begin(); it != roomID.end(); it++) {
            if (it->first == to_string(IDs[i])) {
                if (it->second == 0) 
                    msg += "(Public) ";
                else 
                    msg += "(Private) ";
                msg += "Game Room ";
                msg += it->first;
                if (roomStatus[it->first] == 0) 
                    msg += " is open for players";
                else 
                    msg += " has started playing";
                
                break;
            }
        }
    }
    msg += "\n";

    return msg;
}

string listUser(vector<string> cmd) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    msg = "List Users";
    vector<string> names;
    for (auto it = upw.begin(); it != upw.end(); it++) {
        names.push_back(it->first);
    }
    if (names.empty()) {
        msg += "\nNo Users\n";
        return msg;
    }

    sort(names.begin(), names.end());
    for (int i = 0; i < names.size(); i++) {
        msg += "\n";
        msg += to_string(i + 1);
        msg += ". ";
        for (auto it = uem.begin(); it != uem.end(); it++) {
            if (it->first == names[i]) {
                msg += it->first;
                msg += "<";
                msg += it->second;
                msg += "> ";
                if (ulog[it->first]) 
                    msg += "Online";
                else 
                    msg += "Offline";
        
                break;
            }
        }
    }

    msg += "\n";
    return msg;
}

string join(vector<string> cmd, int id, bool *succeed) {
    string msg;
    int size = cmd.size();
    if (size != 3) {
        msg = "invalid input\n";
        return msg;
    }

    string RID = cmd[2];
    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].inRoom) {
        msg = "You are already in game room ";
        msg += cinfo[id].room;
        msg += ", please leave game room\n";
    }
    else {
        bool exist = false;
        bool open = false;
        for (auto it = roomID.begin(); it != roomID.end(); it++) {
            if (it->first == RID) {
                exist = true;
                if (it->second == 0)
                    open = true;
            }
        }
        if (!exist) {
            msg = "Game room ";
            msg += RID;
            msg += " is not exist\n";
        }
        else if (!open) {
            msg = "Game room is private, please join game by invitation code\n";
        }
        else {
            bool started = false;
            for (auto it = roomStatus.begin(); it != roomStatus.end(); it++) {
                if (it->first == RID) {
                    if (it->second == 1) {
                        started = true;
                    }
                }
            }
            if (started) {
                msg = "Game has started, you can't join now\n";
            }
            else {
                cinfo[id].inRoom = true;
                cinfo[id].room = RID;
                roomMember[RID].push_back(id);
                *succeed = true;
                msg = "You join game room ";
                msg += RID;
                msg += "\n";
            }
        }
    }

    return msg;
}

string invite(vector<string> cmd, int id, bool *succeed) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    string em = cmd[1];
    string RID = cinfo[id].room;
    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (!cinfo[id].inRoom) {
        msg = "You did not join any game room\n";
    }
    else if (!cinfo[id].manager) {
        msg = "You are not private game room manager\n";
    }
    else {
        string invitee;
        for (auto it = uem.begin(); it != uem.end(); it++) {
            if (it->second == em)
                invitee = it->first;
        }
        bool log = false;
        for (auto it = ulog.begin(); it != ulog.end(); it++) {
            if (it->first == invitee && it->second == true)
                log = true;
        }
        if (!log) {
            msg = "Invitee not logged in\n";
        }
        else {
            *succeed = true;
            msg = "You send invitation to ";
            msg += invitee;
            msg += "<";
            msg += em;
            msg += ">\n";
        }
    }

    return msg;
}

string listInvitation(vector<string> cmd, int id) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    msg = "List invitations";
    if (cinfo[id].invitations.empty()) {
        msg += "\nNo Invitations\n";
    }
    else {
        sort(cinfo[id].invitations.begin(), cinfo[id].invitations.end(), cmp);
        int idx = 0;
        set<string> unique;
        for (auto it = cinfo[id].invitations.begin(); it != cinfo[id].invitations.end(); it++) {
            if (roomID.find(it->room) == roomID.end())
                continue;
            if (unique.count(it->room))
                continue;
            
            msg += "\n";
            msg += to_string(++idx);
            msg += ". ";
            msg += it->invitier;
            msg += "<";
            msg += it->email;
            msg += "> invite you to join game room ";
            msg += it->room;
            msg += ", invitation code is ";
            msg += it->code;
            unique.insert(it->room);
        }
        if (idx == 0) {
            msg += "\nNo Invitations";
        }

        msg += "\n";
    }
    
    return msg;
}

string acceptInvite(vector<string> cmd, int id, bool *succeed) {
     string msg;
    int size = cmd.size();
    if (size != 3) {
        msg = "invalid input\n";
        return msg;
    }

    string em = cmd[1];
    string code = cmd[2];
    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].inRoom) {
        msg = "You are already in game room ";
        msg += cinfo[id].room;
        msg += ", please leave game room\n";
    }
    else {
        bool exist = false;
        for (auto it = cinfo[id].invitations.begin(); it != cinfo[id].invitations.end(); it++) {
            if (it->email == em) {
                exist = true;
                if (it->code != code) {
                    msg = "Your invitation code is incorrect\n";
                }
                else {
                    if (roomStatus[it->room]) {
                        msg = "Game has started, you can't join now\n";
                    }
                    else {
                        *succeed = true;
                        cinfo[id].room = it->room;
                        cinfo[id].inRoom = true;
                        roomMember[it->room].push_back(id);
                        msg = "You join game room ";
                        msg += it->room;
                        msg += "\n";
                    }
                }
                break;
            }
        }
        if (!exist) {
            msg = "Invitation not exist\n";
        }
    }
    return msg;
}

string leave(vector<string> cmd, int id, int* condition) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].room == "") {
        msg = "You did not join any game room\n";
    }
    else {
        msg = "You leave game room ";
        msg += cinfo[id].room;
        string RID = cinfo[id].room;
        if (cinfo[id].manager) {
            *condition = 1;
            cinfo[id].manager = false;
        }
        else if (roomStatus[RID]) {
            msg += ", game ends";
            *condition = 2;
        }
        else {
            *condition = 3;
        }
        msg += "\n";
       
    }

    return msg;
}

string start(vector<string> cmd, int id, bool *succeed) {
    string msg;
    int size = cmd.size();
    // if (size != 4) {
    //     msg = "invalid input";
    //     return msg;
    // }

    string rounds = cmd[2];
    string RID = cinfo[id].room;

    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].room == "") {
        msg = "You did not join any game room\n";
    }
    else if (!cinfo[id].manager) {
        msg = "You are not game room manager, you can't start game\n";
    }
    else if (roomStatus[RID] == 1) {
        msg = "Game has started, you can't start again\n";
    }
    else {
        if (size == 4) { // have typed answer
            string answer = cmd[3];
            if (!isNumber(answer) || answer.size() != 4) {
                msg = "Please enter 4 digit number with leading zero\n";
                return msg;
            }
            else {
                // for (int i = 0; i < 4; ++i)
                //     cinfo[id].ans[i] = answer[i];
                roomAns[RID] = answer;
            }
        }
        else { // no answer
            bool used[10] = {false};
            char temp[4];
            for (int i = 0; i < 4; ++i) {
                int num;
                srand(time(NULL));
                do {
                    num = rand() % 10;
                } while (used[num - 1]);
                temp[i] = num + '0';
                used[num - 1] = true;
            }
            roomAns[RID] = temp;
        }
        *succeed = true;
        gameRounds[RID] = stoi(rounds);
        gameGuess[RID] = 0;
        roomStatus[RID] = 1;
        roomIter[RID] = 0;
        msg = "Game start! Current player is ";
        msg += cinfo[id].uname;
        msg += "\n";
    }

    return msg;
}

string game(vector<string> cmd, int id, bool *succeed) {
    string msg;
    int size = cmd.size();
    if (size != 2) {
        msg = "invalid input\n";
        return msg;
    }

    string guess = cmd[1];
    string guess2 = guess;
    string RID = cinfo[id].room;
    if (!cinfo[id].logio) {
        msg = "You are not logged in\n";
    }
    else if (cinfo[id].room == "") {
        msg = "You did not join any game room\n";
    }
    else if (roomStatus[RID] == 0) {
        if (cinfo[id].manager)
            msg = "You are game room manager, please start game first\n";
        else 
            msg = "Game has not started yet\n";
    }
    else {
        int cur = roomIter[RID] % roomMember[RID].size();
        int player = roomMember[RID][cur];
        if (player != id) {
            msg = "Please wait..., current player is ";
            msg += cinfo[player].uname;
            msg += "\n";
        }
        else {
            if (!isNumber(guess) || guess.size() != 4) {
                msg = "Please enter 4 digit number with leading zero\n";
            }
            else {
                bool pos[4] = {false};
                int cntA = 0, cntB = 0;
                for (int i = 0; i < 4; ++i) { // A
                    if (guess[i] == roomAns[RID][i]) {
                        cntA++;
                        pos[i] = true;
                        guess[i] = ' ';
                    }
                }
                for (int i = 0; i < 4; ++i) { // B
                    for (int j = 0; j < 4; ++j) { // ans
                        if (j == i || pos[j]) continue;
                        if (guess[i] == roomAns[RID][j]){
                            cntB++;
                            pos[j] = true;
                            break;
                        }
                    }
                }
                
                *succeed = true;
                roomIter[RID]++;
                if (roomIter[RID] == roomMember[RID].size())
                    gameGuess[RID]++;
                roomIter[RID] %= roomMember[RID].size();
                msg = cinfo[id].uname;
                msg += " guess \'";
                msg += guess2;
                msg += "\' and got ";
                if (cntA == 4) {
                    msg += "Bingo!!! ";
                    msg += cinfo[id].uname;
                    msg += " wins the game, game ends\n";
                    roomStatus[RID] = 0;
                }
                else {
                    msg += "\'";
                    msg += cntA + '0';
                    msg += "A";
                    msg += cntB + '0';
                    msg += "B\'";
                    if (gameGuess[RID] == gameRounds[RID]){
                        msg += "\nGame ends, no one wins";
                        roomStatus[RID] = 0;
                    }
                    msg += "\n";
                    
                }
            }
        }
    }

    return msg;
}

void exitt(int id) {
    if (cinfo[id].inRoom) {
        string RID = cinfo[id].room;
        if (cinfo[id].manager) { // manager leave
            if (roomID[RID])
                roomCode.erase(RID);
            roomID.erase(RID);
            roomStatus.erase(RID);
            roomMember.erase(RID);
        }
        else if (roomStatus[RID]) { // leave while playing
            vector<int>::iterator ptr;
            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                if (*it == id) {
                    ptr = it;
                    break;
                }
            }
            roomMember[RID].erase(ptr);
            roomStatus[RID] = 0;
        }
        else { // leave when waiting
            vector<int>::iterator ptr;
            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                if (*it == id) {
                    ptr = it;
                    break;
                }
            }
            roomMember[RID].erase(ptr);
        }
    }
    ulog[cinfo[id].uname] = false;
    cinfo[id].logio = false;
    memset(cinfo[id].uname, 0, sizeof(cinfo[id].uname));
    cinfo[id].manager = false;
    cinfo[id].inRoom = false;
    cinfo[id].room = "";
    cinfo[id].invitations.clear();
}

vector<string> tokenize(char* buffer){
    vector<string> cmd;

    int condition = 0;
    string tmp = "";
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == ' ') {
            cmd.push_back(tmp);
            tmp = "";
            condition++;
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

void TCPcmd(vector<string> cmd, string &msg, int id) {
    if (cmd[0] == "exit") { // TCP
        cout << "tcp get msg: exit";
        msg = "";
    }
    else if (cmd[0] == "login") { // TCP
        msg = login(cmd, id);
    }
    else if (cmd[0] == "logout") { // TCP
        msg = logout(cmd, id);
    }
    else if (cmd[0] == "create") { // TCP
        if (cmd[1] == "public")
            msg = createPublic(cmd, id);
        else 
            msg = createPrivate(cmd, id);
    }
    else if (cmd[0] == "list") {
        msg = listInvitation(cmd, id);
    }
    else
        msg = "";
}

void UDPcmd(vector<string> cmd, string &msg) {
    if (cmd[0] == "register") { // UDP
        msg = regist(cmd); 
    }
    else if (cmd[0] == "list") {
        if (cmd[1] == "rooms")
            msg = listRoom(cmd);
        else
            msg = listUser(cmd);
    }
    else
        msg = "";
}



// TCP: socket->bind->listen->accept->do something->close
// UDP: socket->bind->receive->close
int main(int argc, char* argv[]) {
    // if (argc != 2) {
    //     cout << "PORT error";
    //     return 0;
    // }
    // int port = atoi(argv[1]);

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
    server.sin_port = htons(PORT);
    
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
        FD_SET(udpfd, &readfds);
        // get maxfd
        int maxfd;
        maxfd = max(listenfd, udpfd);

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
                    // printf("Adding to list of sockets as %d\n" , i);  
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
                if ((valread = recv(connfd, buffer, 1024, 0)) == 0) {
                    // getpeername(connfd , (SA*)&server, &len);  
                    // printf("Host disconnected , ip %s , port %d \n", 
                    //         inet_ntoa(server.sin_addr) , ntohs(server.sin_port));  
                            
                    // Close the socket and mark as 0 in list for reuse
                    exitt(id);
                    // close(connfd);  
                    client_socket[i] = 0;  
                }
              
                //Echo back the message that came in 
                else {
                    // cout << buffer << '\n';
                    vector<string> cmd = tokenize(buffer);
                    if (cmd[0] == "join") { // join
                        bool succeed = false;
                        msg = join(cmd, id, &succeed);
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);

                        if (succeed) {
                            string room = cmd[2];
                            string name = cinfo[id].uname;
                            msg = "Welcome, ";
                            msg += name;
                            msg += " to game!\n";
                            response = msg.c_str();
                            for (auto it = roomMember[room].begin(); it != roomMember[room].end(); it++) {
                                if (*it == id)
                                    continue;
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                        }

                    }
                    else if (cmd[0] == "invite") { // invite
                        bool succeed = false;
                        msg = invite(cmd, id, &succeed);                        
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);

                        if (succeed) {
                            string invitier = cinfo[id].uname;
                            string em = uem[invitier];
                            msg = "You receive invitation from ";
                            msg += invitier;
                            msg += "<";
                            msg += em;
                            msg += ">\n";
                            response = msg.c_str();
                            
                            string invitee;
                            for (auto it = uem.begin(); it != uem.end(); it++) {
                                if (it->second == cmd[1])
                                    invitee = it->first;
                            }
                            for (int CID = 0; CID < MAXCLIENT; CID++) {
                                if (cinfo[CID].uname == invitee) {
                                    invit invi_info;
                                    invi_info.invitier = cinfo[id].uname;
                                    invi_info.email = uem[cinfo[id].uname];
                                    invi_info.room = cinfo[id].room;
                                    invi_info.code = roomCode[cinfo[id].room];
                                    cinfo[CID].invitations.push_back(invi_info);
                                    int tempfd = client_socket[CID];
                                    send(tempfd, response, strlen(response), 0);
                                }
                                
                            }
                        }

                    }
                    else if (cmd[0] == "accept") {
                        bool succeed = false;
                        msg = acceptInvite(cmd, id, &succeed);                      
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);
                        if (succeed) {
                            string name = cinfo[id].uname;
                            string RID = cinfo[id].room;
                            msg = "Welcome, ";
                            msg += name;
                            msg += " to game!\n";
                            response = msg.c_str();

                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                                cout << id << " ";
                                if (*it == id)
                                    continue;
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                        }
                    }
                    else if (cmd[0] == "leave") {
                        int condition = 0;
                        msg = leave(cmd, id, &condition);                      
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);

                        if (condition == 1) { // manager leave
                            string RID = cinfo[id].room;
                            msg = "Game room manager leave game room ";
                            msg += RID;
                            msg += ", you are forced to leave too\n";
                            response = msg.c_str();

                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {\
                                cinfo[*it].inRoom = false;
                                cinfo[*it].room = "";
                                if (*it == id) {
                                    continue;
                                }
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                            cinfo[id].manager = false;
                            if (roomID[RID])
                                roomCode.erase(RID);
                            roomID.erase(RID);
                            roomStatus.erase(RID);
                            roomMember.erase(RID);
                        }
                        else if (condition == 2) {
                            string name = cinfo[id].uname;
                            string RID = cinfo[id].room;
                            cinfo[id].room = "";
                            cinfo[id].inRoom = false;
                            msg = name;
                            msg += " leave game room ";
                            msg += RID;
                            msg += ", game ends\n";
                            response = msg.c_str();
                            
                            vector<int>::iterator ptr;
                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                                if (*it == id) {
                                    ptr = it;
                                    continue;
                                }
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                            roomMember[RID].erase(ptr);
                            roomStatus[RID] = 0;
                        }
                        else if (condition == 3) {
                            string name = cinfo[id].uname;
                            string RID = cinfo[id].room;
                            cinfo[id].room = "";
                            cinfo[id].inRoom = false;
                            msg = name;
                            msg += " leave game room ";
                            msg += RID;
                            msg += "\n";
                            response = msg.c_str();

                            vector<int>::iterator ptr;
                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                                if (*it == id) {
                                    ptr = it;
                                    continue;
                                }
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                            roomMember[RID].erase(ptr);
                            
                        }

                    }
                    else if (cmd[0] == "start") {
                        bool succeed = false;
                        msg = start(cmd, id, &succeed);                      
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);

                        if (succeed) {
                            string RID = cinfo[id].room;
                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                                if (*it == id) {
                                    continue;
                                }
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                        }
                    }
                    else if (cmd[0] == "guess") {
                        bool succeed = false;
                        msg = game(cmd, id, &succeed);                      
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);

                        if (succeed) {
                            string RID = cinfo[id].room;
                            for (auto it = roomMember[RID].begin(); it != roomMember[RID].end(); it++) {
                                if (*it == id) {
                                    continue;
                                }
                                int tempfd = client_socket[*it];
                                send(tempfd, response, strlen(response), 0);
                            }
                        }
                    }
                    
                    else {
                        msg = "";
                        TCPcmd(cmd, msg, id);
                        const char* response = msg.c_str();
                        send(connfd, response, strlen(response), 0);
                    }
                    // cout << msg << '\n';
                    // cout << response << '\n';
                } 
            }
            
        }

        // if udp socket is readable receive the msg.
        if (FD_ISSET(udpfd, &readfds)) {
            string msg;
            len = sizeof(client);
            bzero(buffer, sizeof(buffer));
            n = recvfrom(udpfd, buffer, 1024, 0, (SA*)&client, &len);
            // recv(udpfd, buffer, 1024, 0);
            vector<string> cmd = tokenize(buffer);
            // if (cmd[0] == "list") {
            //     cout << "udp: " << buffer << "@@\n";
            // }
            UDPcmd(cmd, msg);
            // cout << msg << '\n';
            const char* response = msg.c_str();
            sendto(udpfd, response, strlen(response), 0, (SA*)&client, sizeof(client));
        }
    }
    // end program

    return 0;
}

