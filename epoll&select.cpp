/*
    linux下关于select和epoll的实验
    epoll的效率是在是高，而且还不用开新的线程，效率比之前的高了很多
    程序直接在linux下编译即可，需要加上-pthread参数，服务器停止需要在服务器手动输入exit即可。

*/

#include <iostream>
#include <sys/socket.h> //socket相关函数
#include <string.h>     //memset
#include <netinet/in.h> //struct sockaddr_in
#include <unistd.h>     //sleep,usleep,close
#include <sys/select.h> //select的io复用相关函数
#include <thread>       //用来实现accept和io复用之间的协同工作
#include <list>
#include <chrono>
#include <atomic>
#include <fcntl.h>
#include <sys/epoll.h> //epoll复用
using namespace std;
//3种io复用实现的简单TCP服务器
//全局变量区
int socket_fd;               //TCP服务器开启的端口描述符
struct sockaddr_in servaddr; //服务器的网络地址

//设置fd为非阻塞的io
void setNoneBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK; //设置为非阻塞
    fcntl(fd, F_SETFL, flags);
}

//1.用select实现的socket服务器
void socket_server()
{
    cout << "select最大处理:" << FD_SETSIZE << "个描述符" << endl;
    fd_set read_fset;    //由于我们只做服务器，因此只用初始化一个“可读”的fd_set
    char buffer[1024];   //缓冲区用于读取客户端发送的数据
    FD_ZERO(&read_fset); //清空结构体
    int count = 1;       //fd个数，不用size（），提升速度
    atomic<int> maxFD;
    //将本地地址绑定到所创建的套接字上
    if (bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(socket_fd); //绑定失败，关闭套接字
        exit(0);
    }
    //绑定成功，循环监听服务器
    listen(socket_fd, 1000); //1000连接数
    maxFD = socket_fd;
    list<int> socket_table; //用来保存所有已经连接上的客户端fd
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
        .detach(); //线程1：用来获取用户输入用以关闭服务器连接

    cout << "======waiting for client's request======\n";
    //线程2，用来接受用户的连接请求
    std::move(thread([&socket_table, &read_fset, &count, &maxFD] { //用于循环接受客户端的accept，用线程防止阻塞
        //socket_table.push_back(socket_fd);
        while (1)
        { //循环接受来自客户端的连接,将客户端加入到read_fset中
            int connfd;
            struct sockaddr_in client_addr;      //客户端的网络地址
            socklen_t siz = sizeof(client_addr); //网络地址的size大小
            connfd = accept(
                socket_fd,
                (struct sockaddr *)(&client_addr),
                &siz);
            socket_table.push_back(connfd); //将新的连接添加到list中，用于在select中查询
            count++;
            FD_SET(connfd, &read_fset);
            maxFD = max(maxFD.load(), connfd);
            cout << connfd << "连接!" << endl;
        }
    }))
        .detach();

    //线程3，用来通过select来处理每一个连接
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
                FD_SET(xfd, &read_fset); //将目前list里所有的connfd全部设置为感兴趣的fd
                //max_fd=max(max_fd,xfd);
                maxFD = max(maxFD.load(), xfd);
            }

            int ret = select(maxFD.load() + 1, &read_fset, nullptr, nullptr, nullptr); //不断返回刷新，检测是否有新的客户端连接进来
            /*if (ret <= 0)
            {
                cout << "select失败" << endl;
                break;
            }*/
            usleep(1000);
            //总数是list的大小，只监听可读事件，时间设置为nullptr，表示永不超时，阻塞到某个描述符就绪以后才返回
            for (auto it = socket_table.begin(); it != socket_table.end(); it++)
            {
                int xfd = *it;
                //在调用select以后，系统会把这个read_Fset中，没有相应的描述符剔除,FD_ISSET就为false了,通过这样来判断描述符上是否有动作
                if (FD_ISSET(xfd, &read_fset))
                {
                    //如果这个描述符可读，那么就接受这个描述符上的内容
                    ret = recv(xfd, buffer, sizeof(buffer) - 1, 0);
                    //cout<<"ret:"<<ret<<endl;
                    if (ret <= 0)
                    {
                        //收到0字节，说明远端已经关闭，就关闭客户端
                        close(xfd);
                        cout << xfd << "连接已经关闭" << endl;
                        count--;
                        it = socket_table.erase(it);
                        continue;
                    }
                    //显示内容
                    cout << "connfd:【" << xfd << "】:" << buffer << endl; //显示从描述符上获取到的内容
                    memset(buffer, 0, sizeof(buffer));
                }
            }
            FD_ZERO(&read_fset);

            //遍历完所有描述符后，继续进行下一个循环：1.初始化填充0   2，填充感兴趣的fd    3，select有消息的fd
        }
        close(socket_fd);
        cout << socket_fd << " socket closed!" << endl;
        exit(0);
    }))
        .detach();

    while (true)
    {
        cout << "-";
        std::this_thread::sleep_for(std::chrono::seconds(1)); //线程睡眠1s
    }
}
//2,poll实现的服务器
void poll_server()
{
}
//3.epoll实现的服务器
void add_fd(int epollfd, int fd)
{
    //暂时不添加et，lt模式
    struct epoll_event event; //这个是服务器的event
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
        close(socket_fd); //绑定失败，关闭套接字
        exit(0);
    }

    //绑定成功，循环监听服务器
    setNoneBlock(socket_fd); //设定服务器套接字为非阻塞

    listen(socket_fd, 1000);           //1000连接数
    int epoll_fd = epoll_create(1000); //建立epoll的事件表
    struct epoll_event events[1001];

    char buffer[1024]; //消息缓冲区
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
        .detach();               //线程1：用来获取用户输入用以关闭服务器连接
    add_fd(epoll_fd, socket_fd); //添加服务器的fd
    while (1)
    {
        //监听并且加入事件表
        //直接开始epoll，在循环中再开始监听
        int ret = epoll_wait(epoll_fd, events, 1000, -1); //-1表示阻塞到有事件发生，因为和服务器线程在同一个循环
        for (int i = 0; i < ret; i++)
        {
            //遍历事件表中所有事件
            cout<<events[i].data.fd<<" ";
            if (events[i].data.fd == socket_fd)
            {
                //如果是服务器fd
                //就accept看看有没有新连接
                int connfd;
                struct sockaddr_in client_addr;
                socklen_t siz = sizeof(client_addr);
                connfd = accept(socket_fd, (struct sockaddr *)(&client_addr), &siz);
                if (connfd == -1)
                    continue;
                add_fd(epoll_fd, connfd);
                cout << connfd << "新连接" << endl;
                continue;
            }
            else //if (events[i].events & EPOLLIN)
            {
                //否则就是代表客户端fd，直接处理就可以
                memset(buffer, 0, sizeof(buffer));
                int _ret = recv(events[i].data.fd, buffer, 1024, 0);
                if (_ret <= 0)
                {
                    cout << events[i].data.fd << "断开连接" << endl;
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr); //从内核表中删除
                    continue;
                }
                cout << events[i].data.fd << ":" << buffer << endl;
            }
        }

    }
}
int main()
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0); //初始化服务器的描述符
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
    servaddr.sin_port = htons(55229);             //设置端口
    setreuseaddr(socket_fd);
    cout << "服务器socketFd:" << socket_fd << endl; //输出这一行的目的是可以在手动关闭服务器后使用close指令单独关闭从而避免端口重复
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