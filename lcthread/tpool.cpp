/*
    一个CPP11的线程池结构，仿造了JAVA中的写法，直接引用头文件即可，任务类为ThreadPool::job,重写run方法即可
    作者：罗增铖  
    qq : 615893982
*/
#include<iostream>
#include<vector>
#include<string>
#include<windows.h>
#include<mutex>
#include<thread>
#include<queue>
#include<atomic>
#include<condition_variable>
using namespace std;
class ThreadPool {
public:
	class job {
	public:
		virtual void run() { };
	};
private:
	queue<job*> _job;//任务列表
	mutex mutex_global, job_mutex;
	condition_variable cv;
	thread *tlist;
	atomic<int> num_running, n;//任务个数
public:

	ThreadPool(int const &_n=0) {
		//初始化线程池个数
		atomic_init<int>(&n,_n);
		if (_n == 0) {
			atomic_init<int>(&n,this->core_num());
		}
		//cout << this->n;
		atomic_init<int>(&num_running,0);
		tlist = new thread[n];
		for (int i = 0; i < n; i++) {
			tlist[i] = thread([this, i]() {//直接在这里用lambda表达式建立线程

				while (1) {
					unique_lock<mutex> lock(this->job_mutex);
					this->cv.wait(lock, [this] {
						return !this->_job.empty();
					});//等待阻塞线程，直到update中通知状态改变
					if (this->_job.size() != 0)
					{
						this->mutex_global.lock();
						job* job = this->_job.front(); this->_job.pop();
						this->mutex_global.unlock();

						lock.unlock();//释放当前线程
						this->num_running++;
						job->run();
						this->num_running--;
						//this->mutex_global.lock(); this->num_running--;this->mutex_global.unlock();

					}

				}
			});

		}
	}

	void addTask(job* job) {
		this->_job.push(job);
		this->update();

	}
	void update() {
		//更新线程状态
		this->cv.notify_one();
	}
	unsigned int core_num() {
		return std::thread::hardware_concurrency();
	}

};
class job2 : public ThreadPool::job {
public:
	int id;
	job2(int i) {
		this->id = i;
	}
	void run() {
		while (1) {
			cout << id; Sleep(id);
		}
		
	}
};
int main() {
	ThreadPool pool;//不加构造参数默认取系统最大核心数 
	 
	pool.addTask(new job2(1));
	pool.addTask(new job2(2));
	string cmd;
	while(1){
		cin>>cmd;
		if(cmd=="exit")return 0;
	}
	return 0;
}
