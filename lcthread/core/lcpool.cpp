#ifndef _LCPOOL_CPP_
#define _LCPOOL_CPP_
#include "lcpool.h"
int lcpool::check(){
   return std::thread::hardware_concurrency();
}
lcpool::lcpool(int num)
{
   this->running=0;
   this->mMutex.unlock();
   this->pool = new future<int>[num];//新建了num个线程对象
   for(int i=0;i<num;i++){
      this->canuse.push((char)i);//初始化可用线程池位置
   }
}
lcpool::~lcpool(){
   delete[] this->pool;
}

#endif