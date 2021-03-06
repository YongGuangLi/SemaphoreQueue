
#ifndef SEMAPHOREQUEUE_H_
#define SEMAPHOREQUEUE_H_

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <queue>

using namespace std;

template<typename T>
class SemaphoreQueue
{
public:
	explicit SemaphoreQueue(size_t size = 100000);

	~SemaphoreQueue();

	bool push_back(const T data, int msWait = -1);

	bool pop_front(T &data, int msWait = -1);

	int sem_wait_time( sem_t *psem, int mswait);

	inline size_t size();
	
	void clear();
private:
	pthread_mutex_t mutex_;
	queue<T> queData_;
	sem_t enques_;
	sem_t deques_;
    int size_;
};

template<typename T>
SemaphoreQueue<T>::SemaphoreQueue(size_t size)
{
	pthread_mutex_init(&mutex_, NULL);
	sem_init( &enques_,0, size );      //入队信号量初始化为size，最多可容纳size各元素
	sem_init( &deques_,0,0 );          //队列刚开始为空，出队信号量初始为0
    size_ = size;
}

template<typename T>
SemaphoreQueue<T>::~SemaphoreQueue()
{
	pthread_mutex_destroy(&mutex_);
	sem_destroy(&enques_);
	sem_destroy(&deques_);
}

template<typename T>
bool SemaphoreQueue<T>::push_back(const T data, int msWait)
{
	bool status = false;
	if(-1 != sem_wait_time(&enques_, msWait))
	{
		pthread_mutex_lock(&mutex_);
		queData_.push(data);
		sem_post(&deques_);
		pthread_mutex_unlock(&mutex_);
		status = true;
	}
	return status;
}

template<typename T>
bool SemaphoreQueue<T>::pop_front(T &data, int msWait)
{
	bool status = false;
	if(-1 != sem_wait_time(&deques_, msWait))
	{
		pthread_mutex_lock(&mutex_);
		data = queData_.front();
		queData_.pop();
		sem_post(&enques_ );
		pthread_mutex_unlock(&mutex_);
		status = true;
	}
	return status;
}

template<typename T>
int SemaphoreQueue<T>::sem_wait_time( sem_t *psem, int mswait)
{
	int rs = 0;
	if(mswait < 0)      //阻塞，直到 psem 大于
	{
		while((rs = sem_wait(psem)) != 0 && errno == EINTR);
	}
	else
	{
		timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);        //获取当前时间
		ts.tv_sec += (mswait / 1000 );             //加上等待时间的秒数
		ts.tv_nsec += ( mswait % 1000 ) * 1000000; //加上等待时间纳秒数

		//等待信号量，errno==EINTR屏蔽其他信号事件引起的等待中断
		while((rs = sem_timedwait( psem, &ts)) != 0 && errno == EINTR);
	}
	return rs;
}

template<typename T>
size_t SemaphoreQueue<T>::size()
{
	
	pthread_mutex_lock(&mutex_);
	size_t size =  queData_.size(); 
	pthread_mutex_unlock(&mutex_);
	return size;
}

template<typename T>
void SemaphoreQueue<T>::clear()
{
	pthread_mutex_lock(&mutex_); 
	queue<T> empty;
	swap(empty, queData_); 

    sem_init( &enques_,0, size_ );      //入队信号量初始化为size，最多可容纳size各元素
    sem_init( &deques_,0,0 );           //队列刚开始为空，出队信号量初始为0
	pthread_mutex_unlock(&mutex_);
}

#endif
