#include "chatroom.h"

int main(int argc, char *argv[])
{
    //服务器IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    //创建监听socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0) { perror("listener"); exit(-1);}
    printf("listen socket created \n");
    //绑定地址
    const int on=1;
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    if( bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind error");
        exit(-1);
    }
    //监听
    int ret = listen(listener, 5);
    if(ret < 0) { perror("listen error"); exit(-1);}
    printf("Start to listen: %s\n", SERVER_IP);
    //在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0) { perror("epfd error"); exit(-1);}
    printf("epoll created, epollfd = %d\n", epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //往内核事件表里添加事件
    addfd(epfd, listener, true);
    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }
        //处理这epoll_events_count个就绪事件
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );
                char name[100]={};
                recv(clientfd,name,sizeof(name),0);//接收客户端的名字信息

                printf("\n%s connected, socketID : %d\n",
                name,
                clientfd);
                addfd(epfd, clientfd, true);

                // 服务端用list保存用户连接
                clt_map[clientfd]=string(name);
                printf("Now there are %d clients int the chat room\n", (int)clt_map.size());

                // 服务端发送欢迎信息
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if(ret < 0) { perror("send error"); exit(-1); }
            }
            //处理用户发来的消息，并广播，使其他用户收到信息
            else
            {
                int ret = sendBroadcastmessage(sockfd);
                if(ret < 0) { perror("error");exit(-1); }
            }
        }
    }
    close(listener); //关闭socket
    close(epfd);    //关闭内核
    return 0;
}

