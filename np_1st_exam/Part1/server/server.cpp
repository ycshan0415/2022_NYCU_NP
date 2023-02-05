#include<iostream>
#include<cstring>
#include<filesystem>
#include<vector>
#include<fstream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

using namespace std;
using namespace std::filesystem;

#define MAXLINE 1024

struct txt {
    string fname = "";
    char fdata[1024];
};

int cmd(char* str){
    string s = "";
    for (int i = 0; i < strlen(str); ++i) {
        s += str[i];
        if (s == "get-file-list") {
            return 1;
        }
        else if (s == "get-file" && str[i+1] == ' ') {
            return 2;
        }
        // else if (s == "exit")
        //     return 3;
    }
    return 0;
}

string get_file_list(){
    string buff = "Files: ";
    string _path = get_current_dir_name();

    // auto it = directory_iterator(_path);
    directory_iterator it(_path);
    for (auto i : it) {
        string name = i.path().filename();
        buff += name;
        buff += ' ';
    }
    return buff;
}

vector<string> target_file(char* str) {
    vector<string> target;
    string name = "";
    // after get-file
    for(int i = 8; i < strlen(str) - 1; ++i){
        if (str[i] == ' '){
            if (name != "") {
                target.push_back(name);
                name = "";
            }
        }
        else
            name += str[i];
    }
    if (name != "") {
        target.push_back(name);
    }

    return target;
}

vector<txt> get_file(vector<string> target){
    vector<txt> content;
    for (int i = 0; i < target.size(); ++i) {
        txt temp;
        temp.fname = target[i];

        ifstream f(target[i], ios::in);
        if(!f.is_open()){
            cout << "file error\n";
            continue;
        }

        string line = "";
        int idx = 0;
        while (getline(f, line)) {
            for (int j = 0; j < line.size(); j++, idx++)
                temp.fdata[idx] = line[j];
            temp.fdata[idx] = '\n';
            idx++; // next line
            line = "";
        }
        
        f.close();
        content.push_back(temp);
    }
    return content;
}

int main(int argc, char*argv[]){
    if (argc != 2) {
        cout << "PORT error\n";
    }
    int port = atoi(argv[1]);

    int udpfd;
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = port;

    int enable = 1;
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        cout << "SO_REUSEADDR failed\n";
    
    if (bind(udpfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        cout << "Bind error\n";
    }

    socklen_t len;
    len = sizeof(client_addr);
    char send_buf[MAXLINE];
    char recv_buf[MAXLINE];
    
    cout << "UDP server is running\n";
    while(1){
        memset(&recv_buf, 0, sizeof(recv_buf));
        memset(&send_buf, 0, sizeof(send_buf));

        int send_num, recv_num;
        recv_num = recvfrom(udpfd, recv_buf, MAXLINE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&len);
        recv_buf[recv_num] = '\0';
        
        cout << "from client:" << recv_buf;

        int type = cmd(recv_buf);
        if (type == 1) {
            string response = get_file_list();
            for (int i = 0; i < response.size(); ++i)
                send_buf[i] = response[i];
            send_num = sendto(udpfd, send_buf, MAXLINE, 0, (struct sockaddr*)&client_addr, len);
        }
        else if (type == 2) {
            vector<string> target = target_file(recv_buf);
            vector<txt> content = get_file(target);
            for (auto it : content) {               
                //send fname
                memset(&send_buf, 0, sizeof(send_buf));
                for (int i = 0; i < it.fname.size(); ++i)
                    send_buf[i] = it.fname[i];
                send_num = sendto(udpfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&client_addr, len);
                //send fdata
                memset(&send_buf, 0, sizeof(send_buf));
                for (int i = 0; i < strlen(it.fdata); ++i)
                    send_buf[i] = it.fdata[i];
                send_num=sendto(udpfd,send_buf,strlen(send_buf),0,(struct sockaddr*)&client_addr,len);
            }
        }
        // else if (type == 3) {}
    }
    
    return 0;
}