/*
    linux�¹���select��epoll��ʵ��
    epoll��Ч�������Ǹߣ����һ����ÿ��µ��̣߳�Ч�ʱ�֮ǰ�ĸ��˺ܶ�
    ����ֱ����linux�±��뼴�ɣ���Ҫ����-pthread������������ֹͣ��Ҫ�ڷ������ֶ�����exit���ɡ�

*/

#include <iostream>
#include <sys/socket.h> //socket��غ���
#include <string.h>     //memset
#include <netinet/in.h> //struct sockaddr_in
#include <unistd.h>     //sleep,usleep,close
#include <sys/select.h> //select��io������غ���
#include <thread>       //����ʵ��accept��io����֮���Эͬ����
#include <list>
#include <chrono>
#include <atomic>
#include <fcntl.h>
#include <sys/epoll.h> //epoll����
using namespace std;
//3��io����ʵ�ֵļ�TCP������
//ȫ�ֱ�����
int socket_fd;               //TCP�����������Ķ˿�������
struct sockaddr_in servaddr; //�������������ַ

//����fdΪ��������io
void setNoneBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK; //����Ϊ������
    fcntl(fd, F_SETFL, flags);
}

//1.��selectʵ�ֵ�socket������
void socket_server()
{
    cout << "select�����:" << FD_SETSIZE << "��������" << endl;
    fd_set read_fset;    //��������ֻ�������������ֻ�ó�ʼ��һ�����ɶ�����fd_set
    char buffer[1024];   //���������ڶ�ȡ�ͻ��˷��͵�����
    FD_ZERO(&read_fset); //��սṹ��
    int count = 1;       //fd����������size�����������ٶ�
    atomic<int> maxFD;
    //�����ص�ַ�󶨵����������׽�����
    if (bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(socket_fd); //��ʧ�ܣ��ر��׽���
        exit(0);
    }
    //�󶨳ɹ���ѭ������������
    listen(socket_fd, 1000); //1000������
    maxFD = socket_fd;
    list<int> socket_table; //�������������Ѿ������ϵĿͻ���fd
    std::move(thread([] {
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
    }))
        .detach(); //�߳�1��������ȡ�û��������Թرշ���������

    cout << "======waiting for client's request======\n";
    //�߳�2�����������û�����������
    std::move(thread([&socket_table, &read_fset, &count, &maxFD] { //����ѭ�����ܿͻ��˵�accept�����̷߳�ֹ����
        //socket_table.push_back(socket_fd);
        while (1)
        { //ѭ���������Կͻ��˵�����,���ͻ��˼��뵽read_fset��
            int connfd;
            struct sockaddr_in client_addr;      //�ͻ��˵������ַ
            socklen_t siz = sizeof(client_addr); //�����ַ��size��С
            connfd = accept(
                socket_fd,
                (struct sockaddr *)(&client_addr),
                &siz);
            socket_table.push_back(connfd); //���µ�������ӵ�list�У�������select�в�ѯ
            count++;
            FD_SET(connfd, &read_fset);
            maxFD = max(maxFD.load(), connfd);
            cout << connfd << "����!" << endl;
        }
    }))
        .detach();

    //�߳�3������ͨ��select������ÿһ������
    std::move(thread([&socket_table, &read_fset, &buffer, &count, &maxFD] {
        struct timeval timeout = {0, 0};
        while (1)
        {
            //cout<<socket_table.size()<<" ";
            socket_table.push_back(socket_fd);
            //int max_fd=socket_fd;
            while (1)
            {
                if (count > 1)
                    break;
                usleep(10);
            }
            for (auto xfd : socket_table)
            {
                FD_SET(xfd, &read_fset); //��Ŀǰlist�����е�connfdȫ������Ϊ����Ȥ��fd
                //max_fd=max(max_fd,xfd);
                maxFD = max(maxFD.load(), xfd);
            }

            int ret = select(maxFD.load() + 1, &read_fset, nullptr, nullptr, nullptr); //���Ϸ���ˢ�£�����Ƿ����µĿͻ������ӽ���
            /*if (ret <= 0)
            {
                cout << "selectʧ��" << endl;
                break;
            }*/
            usleep(1000);
            //������list�Ĵ�С��ֻ�����ɶ��¼���ʱ������Ϊnullptr����ʾ������ʱ��������ĳ�������������Ժ�ŷ���
            for (auto it = socket_table.begin(); it != socket_table.end(); it++)
            {
                int xfd = *it;
                //�ڵ���select�Ժ�ϵͳ������read_Fset�У�û����Ӧ���������޳�,FD_ISSET��Ϊfalse��,ͨ���������ж����������Ƿ��ж���
                if (FD_ISSET(xfd, &read_fset))
                {
                    //�������������ɶ�����ô�ͽ�������������ϵ�����
                    ret = recv(xfd, buffer, sizeof(buffer) - 1, 0);
                    //cout<<"ret:"<<ret<<endl;
                    if (ret <= 0)
                    {
                        //�յ�0�ֽڣ�˵��Զ���Ѿ��رգ��͹رտͻ���
                        close(xfd);
                        cout << xfd << "�����Ѿ��ر�" << endl;
                        count--;
                        it = socket_table.erase(it);
                        continue;
                    }
                    //��ʾ����
                    cout << "connfd:��" << xfd << "��:" << buffer << endl; //��ʾ���������ϻ�ȡ��������
                    memset(buffer, 0, sizeof(buffer));
                }
            }
            FD_ZERO(&read_fset);

            //�����������������󣬼���������һ��ѭ����1.��ʼ�����0   2��������Ȥ��fd    3��select����Ϣ��fd
        }
        close(socket_fd);
        cout << socket_fd << " socket closed!" << endl;
        exit(0);
    }))
        .detach();

    while (true)
    {
        cout << "-";
        std::this_thread::sleep_for(std::chrono::seconds(1)); //�߳�˯��1s
    }
}
//2,pollʵ�ֵķ�����
void poll_server()
{
}
//3.epollʵ�ֵķ�����
void add_fd(int epollfd, int fd)
{
    //��ʱ�����et��ltģʽ
    struct epoll_event event; //����Ƿ�������event
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNoneBlock(fd);
}
void setreuseaddr(int sock)
{
    int opt;
    opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(&opt)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}
void epoll_server()
{
    if (bind(socket_fd, (struct sockaddr *)(&servaddr), sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(socket_fd); //��ʧ�ܣ��ر��׽���
        exit(0);
    }

    //�󶨳ɹ���ѭ������������
    setNoneBlock(socket_fd); //�趨�������׽���Ϊ������

    listen(socket_fd, 1000);           //1000������
    int epoll_fd = epoll_create(1000); //����epoll���¼���
    struct epoll_event events[1001];

    char buffer[1024]; //��Ϣ������
    std::move(thread([] {
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
    }))
        .detach();               //�߳�1��������ȡ�û��������Թرշ���������
    add_fd(epoll_fd, socket_fd); //��ӷ�������fd
    while (1)
    {
        //�������Ҽ����¼���
        //ֱ�ӿ�ʼepoll����ѭ�����ٿ�ʼ����
        int ret = epoll_wait(epoll_fd, events, 1000, -1); //-1��ʾ���������¼���������Ϊ�ͷ������߳���ͬһ��ѭ��
        for (int i = 0; i < ret; i++)
        {
            //�����¼����������¼�
            cout<<events[i].data.fd<<" ";
            if (events[i].data.fd == socket_fd)
            {
                //����Ƿ�����fd
                //��accept������û��������
                int connfd;
                struct sockaddr_in client_addr;
                socklen_t siz = sizeof(client_addr);
                connfd = accept(socket_fd, (struct sockaddr *)(&client_addr), &siz);
                if (connfd == -1)
                    continue;
                add_fd(epoll_fd, connfd);
                cout << connfd << "������" << endl;
                continue;
            }
            else //if (events[i].events & EPOLLIN)
            {
                //������Ǵ���ͻ���fd��ֱ�Ӵ���Ϳ���
                memset(buffer, 0, sizeof(buffer));
                int _ret = recv(events[i].data.fd, buffer, 1024, 0);
                if (_ret <= 0)
                {
                    cout << events[i].data.fd << "�Ͽ�����" << endl;
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr); //���ں˱���ɾ��
                    continue;
                }
                cout << events[i].data.fd << ":" << buffer << endl;
            }
        }

    }
}
int main()
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0); //��ʼ����������������
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //IP��ַ���ó�INADDR_ANY,��ϵͳ�Զ���ȡ������IP��ַ��
    servaddr.sin_port = htons(55229);             //���ö˿�
    setreuseaddr(socket_fd);
    cout << "������socketFd:" << socket_fd << endl; //�����һ�е�Ŀ���ǿ������ֶ��رշ�������ʹ��closeָ����رմӶ�����˿��ظ�
    int type;
    cin >> type;
    switch (type)
    {
    case 0:
    {
        socket_server();
        break;
    }
    case 1:
    {
        poll_server();
        break;
    }
    case 2:
    {
        epoll_server();
        break;
    }
    }
    return 0;
}