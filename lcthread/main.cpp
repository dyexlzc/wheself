#include <iostream>
#include <Windows.h>
#include "core/lcpool.h"
using namespace std;
//线程测试
int main()
{
    //线程池所有的线程均是detach类型
    lcpool pool(5);
    cout << "physical core threads : " << pool.check() << endl;
    //采用手动指定函数的方法，来测试一下,第一是用固定函数，第二使用lambda表达式
    pool.addtask(  [](int id,int n) { 
        for(int i=0;i<n;i++){int pp;
            for(int j=i+1;j<n;j++){
                pp=i+j;
            }
            Sleep(100);
            cout<<id<<" ";
        }
        return 0;
    },1,10);
    pool.addtask(  [](int id,int n) { 
        string pstr[n];
        for(int i=0;i<n;i++){
            cout<<id<<" ";
            pstr[i] =  string("Life is short, you need Python.");
            Sleep(145);
        }
       // cout<<endl;
        return 0;
    },2,20);
    pool.addtask(  [](int id,int n) { 
        for(int i=0;i<n;i++){int pp;
            for(int j=i+1;j<n;j++){
                pp=i+j;
               
            }
            Sleep(120);
           cout<<id<<" ";
        }
        return 0;
    },3,15);
    pool.addtask(  [](int id,int n) { 
        for(int i=0;i<n;i++){int pp;
            for(int j=i+1;j<n;j++){
                pp=i+j;
            }
            Sleep(100);
           cout<<id<<" ";
        }
        return 0;
    },4,10);

    pool.run();//主要是判断线程是否全部执行完毕,其会阻塞main函数直到所有线程执行完毕
    cout << "threads has done!" << endl;

    system("pause");
    return 0;
}