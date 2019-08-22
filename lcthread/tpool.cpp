/*
    һ��CPP11���̳߳ؽṹ��������JAVA�е�д��
    ���ߣ�������  
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
	queue<tjob*> job;//�����б�
	mutex mutex_global,job_mutex;
	condition_variable cv;
	thread *tlist;
	int num_running,n;//�������
	bool if_init;
	tpool(int const &_n):n(_n) {
		//��ʼ���̳߳ظ���
		if_init = false;
		num_running = 0;
		//job_mutex = new mutex[_n];
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
			tlist[i]=thread([this,i]() {//ֱ����������lambda���ʽ�����߳�
				
				while (1) {
					unique_lock<mutex> lock(this->job_mutex);
					//while(this->if_init==false)
					this->cv.wait(lock, [this] {
						return !this->job.empty();
					});//�ȴ������̣߳�ֱ��update��֪ͨ״̬�ı�
					if (this->job.size() != 0)
					{
						this->mutex_global.lock();
						tjob* job = this->job.front(); this->job.pop();
						this->mutex_global.unlock();
						lock.unlock();
						this->num_running++;
						job->run();
						this->num_running--;
						cout << i<<"�߳��������,��ǰ����"<<this->num_running<<"/"<<this->n<<"������������л�ʣ��"<<this->job.size()<<"������δ����" << endl;

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
		//��ʼ���̳߳ظ���
		if_init = false;
		num_running = 0;
		//job_mutex = new mutex[_n];
		tlist = new thread[_n];
		for (int i = 0; i < _n; i++) {
			tlist[i] = thread([this, i]() {//ֱ����������lambda���ʽ�����߳�

				while (1) {
					unique_lock<mutex> lock(this->job_mutex);
					//while(this->if_init==false)
					this->cv.wait(lock, [this] {
						return !this->job.empty();
					});//�ȴ������̣߳�ֱ��update��֪ͨ״̬�ı�
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
						cout << i << "�߳��������,��ǰ����" << this->num_running << "/" << this->n << "������������л�ʣ��" << this->job.size() << "������δ����" << endl;

						//this->if_init = false;
					}
					//delete this->job_mutex;
				}
			});

		}
	}
	void addTask(tjob& job) {
		//cout << "�������" << endl;
		this->job.push(&job);
		this->update();
		
	}
	void update() {
		//�����߳�״̬
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
		//cout << "��Ҫ����"<<this->time<<"ms." << endl;
		Sleep(this->time);
	}
};
int main() {
	tpool pool;//���ӹ������Ĭ��ȡϵͳ�������� 
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