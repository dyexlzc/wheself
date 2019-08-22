/*
    一个CPP11的线程池结构，仿造了JAVA中的写法
    作者：罗增铖  
    qq : 615893982
*/
#include<iostream>
#include<windows.h>
#include<vector>
#include<mutex>
#include<thread>
#include<queue>
#include<condition_variable>
#include<string>
using namespace std;
class tpool {
public:
	class tjob {
	public:
		virtual void run() { cout << "tjob" << endl; };
	};
	queue<tjob*> job;//任务列表
	mutex mutex_global,job_mutex;
	condition_variable cv;
	thread *tlist;
	int num_running,n;//任务个数
	bool if_init;
	tpool(int const &_n):n(_n) {
		//初始化线程池个数
		if_init = false;
		num_running = 0;
		//job_mutex = new mutex[_n];
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
			tlist[i]=thread([this,i]() {//直接在这里用lambda表达式建立线程
				
				while (1) {
					unique_lock<mutex> lock(this->job_mutex);
					//while(this->if_init==false)
					this->cv.wait(lock, [this] {
						return !this->job.empty();
					});//等待阻塞线程，直到update中通知状态改变
					if (this->job.size() != 0)
					{
						this->mutex_global.lock();
						tjob* job = this->job.front(); this->job.pop();
						this->mutex_global.unlock();
						lock.unlock();
						this->num_running++;
						job->run();
						this->num_running--;
						cout << i<<"线程运行完毕,当前运行"<<this->num_running<<"/"<<this->n<<"任务，任务队列中还剩余"<<this->job.size()<<"个任务未处理" << endl;

						//this->if_init = false;
					}
					//delete this->job_mutex;
				}
			});

		}
	}
	tpool() {
		int _n = this->core_num();
		this->n = _n;
		//初始化线程池个数
		if_init = false;
		num_running = 0;
		//job_mutex = new mutex[_n];
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
			tlist[i] = thread([this, i]() {//直接在这里用lambda表达式建立线程

				while (1) {
					unique_lock<mutex> lock(this->job_mutex);
					//while(this->if_init==false)
					this->cv.wait(lock, [this] {
						return !this->job.empty();
					});//等待阻塞线程，直到update中通知状态改变
					if (this->job.size() != 0)
					{
						this->mutex_global.lock();
						tjob* job = this->job.front(); this->job.pop();
						lock.unlock();
						this->num_running++;
						this->mutex_global.unlock();


						job->run();


						this->mutex_global.lock();
						this->num_running--;
						this->mutex_global.unlock();
						cout << i << "线程运行完毕,当前运行" << this->num_running << "/" << this->n << "任务，任务队列中还剩余" << this->job.size() << "个任务未处理" << endl;

						//this->if_init = false;
					}
					//delete this->job_mutex;
				}
			});

		}
	}
	void addTask(tjob& job) {
		//cout << "添加任务" << endl;
		this->job.push(&job);
		this->update();
		
	}
	void update() {
		//更新线程状态
		this->cv.notify_one();
	}
	unsigned int core_num(){
		return std::thread::hardware_concurrency();
	}

};
class job2 : public tpool::tjob {
public:
	DWORD time;
	job2(DWORD time) {
		this->time = time;
	}
	void run() {
		//cout << "我要运行"<<this->time<<"ms." << endl;
		Sleep(this->time);
	}
};
int main() {
	tpool pool;//不加构造参数默认取系统最大核心数 
	//pool.addTask(*new job1());
	pool.addTask(*new job2(500));
	pool.addTask(*new job2(1000));
	string cmd;
	while (1) {
		cin >> cmd;
		if (cmd == "exit")exit(0);
		if(cmd=="a")pool.addTask(*new job2(2000));
	}
	return 0;
}