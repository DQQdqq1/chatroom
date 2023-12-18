#include "chatroom.h"
using namespace std;


void* receive(void* arg){
    int *temp=((int*)arg);
    int sock=*temp;
    while(true){
        char recvBuf[BUF_SIZE] = {};
        int reLen = recv(sock, recvBuf, BUF_SIZE, 0);
        cout<<endl<<recvBuf<<endl;
    }
    pthread_exit(NULL);
}

int main()
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    if(connect(sock,(struct sockaddr *)&server,sizeof(server)) < 0)
    {
        perror("connect");
        return 2;
    }
    string name;
    cout<<"Enter name:";
    getline(cin,name);
    write(sock,(char*)name.c_str(),name.length());
    void* temp=&sock;
    pthread_t th;
    pthread_create(&th,NULL,receive,temp);
    while(true){
        string s;
        getline(cin,s);
        write(sock,(char*)s.c_str(),s.length());
    }

    close(sock);
    return 0;
}

