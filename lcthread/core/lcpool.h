#ifndef _LCPOOL_H_
#define _LCPOOL_H_
#include <thread>
#include <vector>
#include <stack>
#include <iostream>
#include <queue>
#include <future>
#include <condition_variable>
#include <mutex>
using namespace std;
class lcpool
{
    public:
    std::future<int> *pool; //线程池对象
    int running;
    stack<char> canuse; //当前可用线程
    queue<thread> wait; //等待队列
    std::mutex mMutex;  //互斥锁

    int check();
    lcpool(int num);
    ~lcpool();

    void run()
    {
        while ( running!=0){}
    }
    template <class F, class... P>
    int addtask(F const &&f, P &&... pa)
    { //第一个参数是所需要执行的函数指针，第二个参数是可变参数，里面是需要传递给函数的参数
        /*if (canuse.empty())
        {
            //如果当前池子满了，就将线程放入wait队列中等待
            wait.push([&](F const &&f, P &&... pa) {
                pool[n] = std::async(std::launch::deferred, f, std::forward<P>(pa)...);
                if (pool[n].get() == 0) //如果进程结束了
                {
                    mMutex.lock();
                    canuse.push(n);
                    running--;
                    mMutex.unlock();
                    cout << n << " has donw!" << endl;
                }
            },
                      f, std::forward<P>(pa)...);
        }*/
        // pool[canuse.top()] = thread(f, std::forward<P>(pa)...); //使用std::forward进行右值转发的功能，将外部可变参数转发给lambda
        int n = canuse.top();
        canuse.pop();
        mMutex.lock();
        running++;
        mMutex.unlock();
        //使用一个新线程来管理这个用户线程
        thread t([&,n](F const &&f, P &&... pa) {
            pool[n] = std::async(std::launch::async, f, std::forward<P>(pa)...);
            if (pool[n].get() == 0) //如果进程结束了
            {
                mMutex.lock();
                canuse.push(n);//资源放回线程池
                running--;
                mMutex.unlock();
                cout << "No. "<<n << " has donw! current running:"<<running << endl;
            }
        },
                 f, std::forward<P>(pa)...);
        //cout << t.get_id() << " new finished!" << endl;
        t.detach();//分离线程，自动回收内存
    }
};
#endif