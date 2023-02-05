#include<iostream>
#include<string>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<arpa/inet.h>

using namespace std;

#define MAXLINE 1024
#define MAXUSER 10

struct shm{
    int number;
    bool user[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
};

key_t key = ftok("sharedMem", 1);
int shmid = shmget(key, sizeof(shm), 0666|IPC_CREAT);

string get_user_num() {
    shm*ptr = (shm*)shmat(shmid, NULL, 0);
    ptr->number++;
    string num = to_string(ptr->number);
    shmdt(ptr);
    return num;
}

void serverOutput(struct sockaddr_in client_addr, string num, int action){
    uint16_t port = ntohs(client_addr.sin_port);
    // 0 for new connection
    // 1 for exit
    if (action == 0)
        cout << "New connection from " << inet_ntoa(client_addr.sin_addr) <<":" << port << " user" << num << "\n";
    else if (action == 1)
        cout << "user" << num << " " << inet_ntoa(client_addr.sin_addr) << ":" << port << " disconnected\n";
}

void userStatus(string num, int action) {
    shm* ptr = (shm*)shmat(shmid, NULL, 0);
    bool* puser = ptr->user;
    if (action == 0)
        puser[stoi(num)] = true;
    else if (action == 1)
        puser[stoi(num)] = false;
    shmdt(ptr);
}

string get_user_list() { // cmd 1
    string msg = "";
    shm* ptr = (shm*)shmat(shmid, NULL, 0);
    bool* puser = ptr->user;
    for (int i = 0; i < 10; i++) {
        if (puser[i] == 1){
            msg += ("user" + to_string(i) + "\n");
        }
    }
    msg[msg.size() - 1] = '\0';
    shmdt(ptr);
    return msg;
}

string get_ip(struct sockaddr_in client_addr){
    string msg = "IP: ";
    string ip = inet_ntoa(client_addr.sin_addr);
    msg += ip;
    msg += ":";
    msg += to_string(ntohs(client_addr.sin_port));
    return msg;
}

int cmd(char* str) {
    string s = "";
    size_t len = strlen(str);
    for (int i = 0; i < strlen(str); ++i) {
        s += str[i];
        if (s == "list-users") {
            return 1;
        }
        else if (s == "get-ip") {
            return 2;
        }
        else if (s == "exit")
            return 3;
    }
    return 0;
}

int main(int argc, char*argv[]) {
    shmctl(shmid, IPC_RMID, NULL);
    key = ftok("sharedMem", 1);
    shmid = shmget(key, sizeof(shm), 0666|IPC_CREAT);

    if (argc != 2) {
        cout << "PORT error\n";
        return 0;
    }
    int port = atoi(argv[1]);

    int tcpfd;
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = port; // htons()

    int enable = 1;
    if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        cout << "SO_REUSEADDR failed\n";
        return 0;
    }

    if (bind(tcpfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Bind error\n";
        return 0;
    }
    if (listen(tcpfd, MAXUSER) < 0) {
        cout << "Listen error\n";
        return 0;
    }

    socklen_t len;
    len = sizeof(server_addr);
    int connfd;
    char send_buff[MAXLINE], recv_buff[MAXLINE];
    
    cout << "TCP server is running\n";
    shm*ptr = (shm*)shmat(shmid,NULL,0);
    ptr->number = 0;
    shmdt(ptr);
    
    while (1) {
        connfd = accept(tcpfd, (struct sockaddr*)&client_addr, (socklen_t*)&len);

        pid_t pid;
        pid = fork();
        if (pid == 0) { // child
            close(tcpfd);
            string num = get_user_num(); // num of user
            serverOutput(client_addr, num, 0);
            userStatus(num, 0);
            while (1) {
                memset(&recv_buff, 0, sizeof(recv_buff));
                memset(&send_buff, 0, sizeof(send_buff));

                int send_num, recv_num;
                recv_num = recv(connfd, recv_buff, MAXLINE, 0);
                recv_buff[recv_num] = '\0';
                // cout << "from client: " << recv_buff;
                int type = cmd(recv_buff);
                if (type == 1) { // get-user-list
                    string msg = get_user_list();
                    strcpy(send_buff, msg.c_str());
                    send_num = send(connfd, send_buff, strlen(send_buff), 0);
                }
                else if (type == 2) { // get-ip
                    string msg = get_ip(client_addr);
                    strcpy(send_buff, msg.c_str());
                    send_num = send(connfd, send_buff, strlen(send_buff), 0);
                }
                else if (type == 3) { // exit
                    string msg = "Bye, user" + num + ".";
                    strcpy(send_buff, msg.c_str());
                    send_num = send(connfd, send_buff, strlen(send_buff), 0);
                    userStatus(num, 1);
                    break;
                }
                else{
                    cout << "cmd error\n";
                    char msg[] = "";
                    send_num = send(connfd, msg, strlen(msg), 0);
                }
            }
            serverOutput(client_addr, num, 1); // leave
            close(connfd);
            exit(0);
        }
        else // parent 
            close(connfd);
        
    }
    // won't reach here
    close(tcpfd);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}