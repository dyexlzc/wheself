/*
    一个CPP11的线程池结构，仿造了JAVA中的写法，直接引用头文件即可，任务类为ThreadPool::job,重写run方法即可
    作者：罗增铖  
    qq : 615893982
*/
#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include<mutex>
#include<thread>
#include<queue>
#include<condition_variable>
#include<string>
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
	int num_running, n;//任务个数
public:

	ThreadPool(int const &_n) :n(_n) {
		//初始化线程池个数
		num_running = 0;
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
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
						lock.unlock();
						this->num_running++;this->mutex_global.unlock();
						job->run();
						this->mutex_global.lock(); this->num_running--;this->mutex_global.unlock();

					}

				}
			});

		}
	}
	ThreadPool() {
		int _n = this->core_num();
		this->n = _n;
		//初始化线程池个数
		num_running = 0;
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
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
						lock.unlock();
						this->num_running++;
						this->mutex_global.unlock();

						job->run();

						this->mutex_global.lock();
						this->num_running--;
						this->mutex_global.unlock();
					}
				}
			});

		}
	}
	void addTask(job& job) {
		this->_job.push(&job);
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
#endif
