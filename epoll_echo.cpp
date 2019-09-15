/*
    使用epoll构建的一个回显服务器
    编译环境    deepin 15.4 + gcc 6.3.0 + pthread
    g++ echo.cpp -o echo -pthread
*/
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <netinet/in.h> //struct sockaddr_in
#include <thread>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>//提供了显示ip转换的功能
using namespace std;
int socket_fd;
struct sockaddr_in serv_addr;
socklen_t siz;
void setnoneBlock(int fd){
    //将fd所对应的描述符设置成非阻塞的,因为epoll要求
    int flags=fcntl(fd,F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
}
void add_fd(int epoll_fd,int fd){
    //往epoll内核表中添加待监视fd
    //默认为电平触发Level Trigger
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN | EPOLLOUT;//可读可写
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    setnoneBlock(fd);//设置为非阻塞
}
void epoll(){
    int epoll_fd=epoll_create(5000); //创建epoll内核表
    add_fd(epoll_fd,socket_fd);//先把服务器的fd加进去
    struct epoll_event events[5000];
    char buffer[1024];//读取缓冲区
    int ret;
    while(1){
        int nfds=epoll_wait(epoll_fd,events,5000,-1);//-1阻塞
        if(nfds<0){
            cout<<"【ERROR】epoll错误"<<endl;
            close(epoll_fd);
            close(socket_fd);
            return;
        }
        for(int i=0;i<nfds;i++){ //有描述符就绪了，遍历每一个就绪的描述符
           int tmp_fd=events[i].data.fd;
           if(tmp_fd==socket_fd){
               //如果是服务器的fd，就看上面有没有可读的事件
               int connfd;
               struct sockaddr_in client_addr;
               socklen_t siz=sizeof(client_addr);
               connfd=accept(socket_fd,(struct sockaddr*)(&client_addr),&siz);
               if(connfd==-1){
                   continue;
               }
               add_fd(epoll_fd,connfd);
               cout<<"【INFO】新连接:"<<inet_ntoa(client_addr.sin_addr)<<":"<<client_addr.sin_port<<endl;
           }
           else if((events[i].events & EPOLLIN)&&(events[i].events&EPOLLOUT) ){ //如果客户端可读可写
                memset(buffer,0,sizeof(buffer));
                ret=recv(tmp_fd,buffer,1024,0);
                if(ret<=0){//如果不可读
                    cout<<"【ERROR】客户端"<<tmp_fd<<"关闭了"<<endl;
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,tmp_fd,nullptr);//从内核表中删除该描述符
                    close(tmp_fd);
                    continue;
                }
                //如果可读，就在服务器显示客户端发送来的消息，并且发送返回给客户端
                send(tmp_fd,buffer,sizeof(buffer),0);
                cout<<tmp_fd<<":"<<buffer<<endl;
                
           }
        }
    }
}
int main(){
    cout<<"echo server is starting...."<<endl;
    int port=58798;
    //启动服务器
    socket_fd=socket(AF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(socket_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(socket_fd); //绑定失败，关闭套接字
        exit(0);
    }
    //开始监听客户端
    listen(socket_fd,5000);//监听5000个吧~

    move(thread([]{//退出服务器的脚本
        string cmd;
        while (1)
        {
            cin >> cmd;
            if (cmd == "exit")
            {
                close(socket_fd);
                cout << socket_fd << " socket closed!" << endl;
                exit(0);
            }
        }
    })).detach();
    epoll();//开始epoll
    return 0;
}