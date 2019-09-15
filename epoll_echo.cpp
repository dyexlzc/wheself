/*
    ʹ��epoll������һ�����Է�����
    ���뻷��    deepin 15.4 + gcc 6.3.0 + pthread
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
#include <arpa/inet.h>//�ṩ����ʾipת���Ĺ���
using namespace std;
int socket_fd;
struct sockaddr_in serv_addr;
socklen_t siz;
void setnoneBlock(int fd){
    //��fd����Ӧ�����������óɷ�������,��ΪepollҪ��
    int flags=fcntl(fd,F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
}
void add_fd(int epoll_fd,int fd){
    //��epoll�ں˱�����Ӵ�����fd
    //Ĭ��Ϊ��ƽ����Level Trigger
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN | EPOLLOUT;//�ɶ���д
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    setnoneBlock(fd);//����Ϊ������
}
void epoll(){
    int epoll_fd=epoll_create(5000); //����epoll�ں˱�
    add_fd(epoll_fd,socket_fd);//�Ȱѷ�������fd�ӽ�ȥ
    struct epoll_event events[5000];
    char buffer[1024];//��ȡ������
    int ret;
    while(1){
        int nfds=epoll_wait(epoll_fd,events,5000,-1);//-1����
        if(nfds<0){
            cout<<"��ERROR��epoll����"<<endl;
            close(epoll_fd);
            close(socket_fd);
            return;
        }
        for(int i=0;i<nfds;i++){ //�������������ˣ�����ÿһ��������������
           int tmp_fd=events[i].data.fd;
           if(tmp_fd==socket_fd){
               //����Ƿ�������fd���Ϳ�������û�пɶ����¼�
               int connfd;
               struct sockaddr_in client_addr;
               socklen_t siz=sizeof(client_addr);
               connfd=accept(socket_fd,(struct sockaddr*)(&client_addr),&siz);
               if(connfd==-1){
                   continue;
               }
               add_fd(epoll_fd,connfd);
               cout<<"��INFO��������:"<<inet_ntoa(client_addr.sin_addr)<<":"<<client_addr.sin_port<<endl;
           }
           else if((events[i].events & EPOLLIN)&&(events[i].events&EPOLLOUT) ){ //����ͻ��˿ɶ���д
                memset(buffer,0,sizeof(buffer));
                ret=recv(tmp_fd,buffer,1024,0);
                if(ret<=0){//������ɶ�
                    cout<<"��ERROR���ͻ���"<<tmp_fd<<"�ر���"<<endl;
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,tmp_fd,nullptr);//���ں˱���ɾ����������
                    close(tmp_fd);
                    continue;
                }
                //����ɶ������ڷ�������ʾ�ͻ��˷���������Ϣ�����ҷ��ͷ��ظ��ͻ���
                send(tmp_fd,buffer,sizeof(buffer),0);
                cout<<tmp_fd<<":"<<buffer<<endl;
                
           }
        }
    }
}
int main(){
    cout<<"echo server is starting...."<<endl;
    int port=58798;
    //����������
    socket_fd=socket(AF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(socket_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(socket_fd); //��ʧ�ܣ��ر��׽���
        exit(0);
    }
    //��ʼ�����ͻ���
    listen(socket_fd,5000);//����5000����~

    move(thread([]{//�˳��������Ľű�
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
    epoll();//��ʼepoll
    return 0;
}