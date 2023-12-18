#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <cstring>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define EPOLL_SIZE 5000
#define BUF_SIZE 0xFFFF
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"
#define SERVER_MESSAGE "ClientID %d: name:%s >> %s"
#define EXIT "EXIT"
#define CAUTION "There is only one int the char room!"

int setnonblocking(int sockfd);
void addfd( int epollfd, int fd, bool enable_et );
int sendBroadcastmessage(int clientfd);

using namespace std;

map<int,string> clt_map;//储存客户信息，socketfd和客户名字
int setnonblocking(int sockfd)//非阻塞模式设置
{
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)| O_NONBLOCK);
    return 0;
}

void addfd( int epollfd, int fd, bool enable_et )//将fd加入到epoll中，并设置边缘触发模式
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
    printf("fd added to epoll!\n\n");
}

string private_msg(string& name){//私信时处理输入字符串
    string msg="";
    if(name[0]=='@'){
        for(int i=1;i<name.length();i++){
            if(name[i]=='@'){
                msg+=name.substr(i+1);
                name=name.substr(1,i-1);
                break;
            }
        }
    }
    return msg;
}
int sendBroadcastmessage(int clientfd)//处理从客户端输入的信息
{
    // buf[BUF_SIZE] receive new chat message
    // message[BUF_SIZE] save format message
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);

    // receive message
    printf("read from client(clientID = %d)\n", clientfd);
    int len = recv(clientfd, buf, BUF_SIZE, 0);

    if(len == 0)  // len = 0 means the client closed connection
    {
        close(clientfd);
        clt_map.erase(clientfd);
        printf("ClientID = %d closed.\n now there are %d client in the char room\n", clientfd, clt_map.size());

    }
    if(clt_map.size() == 1) { // this means There is only one int the char room
        send(clientfd, CAUTION, strlen(CAUTION), 0);
        return len;
    }
    if(0==strcmp("cl",buf)){//获取当前在线的client列表
        for(auto i:clt_map){
            char t[BUF_SIZE]={};
            sprintf(t,"Socket ID: %d -> Client Name: %s",i.first,(char*)i.second.c_str());
            if( send(clientfd, t, BUF_SIZE, 0) < 0 ) { perror("error"); exit(-1);}
        }
    }
    else  //broadcast message
    {
        string name(buf);
        string msg=private_msg(name);
        if(name==clt_map[clientfd]){//输入的目标客户端名字就是该客户端的名字时
            string nouser="Can't send message to yourself.";
            if( send(clientfd, (char*)nouser.c_str(), nouser.length(), 0) < 0 ) { perror("error"); exit(-1);}
            return len;
        }
        int tarfd=-1;
        for(auto i:clt_map){//将客户端输入的私信信息输入到目标客户端上
            if(i.second==name){
                tarfd=i.first;
                sprintf(message,"Message from %s: %s",(char*)clt_map[clientfd].c_str(),(char*)msg.c_str());
                if( send(i.first, message, BUF_SIZE, 0) < 0 ) { perror("error"); exit(-1);}
                return len;
            }
        }
        if(tarfd==-1&&msg!=""){//当客户端直接进行群聊时进行广播
            string nouser="No client named "+name;
            if( send(clientfd, (char*)nouser.c_str(), nouser.length(), 0) < 0 ) { perror("error"); exit(-1);}
            return len;
        }
        // format message to broadcast
        sprintf(message, SERVER_MESSAGE, clientfd,(char*)clt_map[clientfd].c_str(), buf);

        list<int>::iterator it;
        for(auto it : clt_map) {
           if(it.first != clientfd){
                if( send(it.first, message, BUF_SIZE, 0) < 0 ) { perror("error"); exit(-1);}
           }
        }
    }
    return len;
}


