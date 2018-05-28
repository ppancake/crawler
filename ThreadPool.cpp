//
// Created by zwk on 18-5-28.
//

#include <iostream>
#include <time.h>
#include "ThreadPool.h"
#include <errno.h>
#include <cstdlib>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>


using std::cout;
using std::endl;
const time_t maxWaitTime=10;
extern  int anotherSrv;
extern sockaddr_in srvaddr2;






void *threadRoutine(void* arg);

//为防止粘包，采取定长包方式
int recvPack(int sockfd,char *buf,int len )
{
    int leftlen=len;//剩余长度
    char* p=buf;
    int recvlen;
    int ret=0;
    if(buf==NULL)
        return -2;
    while(leftlen>0)
    {
        recvlen=recv(sockfd,p,leftlen, MSG_NOSIGNAL);
        if(recvlen<=0)
        {
            if(errno==EAGAIN)  //由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读,在这里就当作是该次事件已处理过。
                break;
            else if(recvlen<0)
            {
                cout<<"读取错误,对方关闭"<<endl;
                close(sockfd);
            }
            break;
//    		return recvlen;
        }
        p=p+recvlen;//缓冲区指针前移
        leftlen=leftlen-recvlen;//剩余长度变短
        ret=ret+recvlen;
    }
    return ret;
}



//初始化线程池
void ThreadPool::initThreadPool()
{
    pthread_t id=0;
    int ret=0;
    this->cond.lock();//创建线程时要加锁
    this->urlQueue.push(seedUrl);
    for(int i=0;i<this->initThreadNum;i++)
    {

        ret=pthread_create(&id,NULL,&threadRoutine,(void *)this);//id会引用改成现在的id值
        if(ret==0)//返回0,线程创建成功，线程ID入队
        {
            this->threadQueue.push(id);
            this->nowThreadNum++;
        }
    }

    this->cond.unlock();
}
void ThreadPool::destroyThreadPool()//销毁线程池
{
    this->cond.lock();
    if(!isDestroy)
        isDestroy=true;
    if(this->idleThreadNum>0)
        this->cond.broadcast();
    if(this->nowThreadNum>0)
    {
        while(this->nowThreadNum>0)
        {
            this->cond.wait();
        }
    }
    //根据线程id销毁
    while(!this->threadQueue.empty())
    {
        pthread_join(this->threadQueue.front(),nullptr);
        this->threadQueue.pop();
    }


    this->cond.unlock();
}

void *threadRoutine(void* arg)//线程入口函数
{
    ThreadPool *threadpool=(ThreadPool *)arg;
    timespec timeOut = {0};
    int flag=1;
    while(flag)//这个循环什么时候退出？
    {
        threadpool->cond.lock();

        //创建了一个新线程，空闲+1
        (threadpool->idleThreadNum)++;
        //等到任务到来或者销毁通知
        while(threadpool->urlQueue.empty()&& !threadpool->isDestroy)
        {
//            cout<<"Thread:"<<(int)pthread_self()<<"is waiting"<<endl;
            clock_gettime(CLOCK_REALTIME, &timeOut); //设置超时时间
            timeOut.tv_sec+=maxWaitTime;
            if(threadpool->cond.timedwait(timeOut)==ETIMEDOUT)
            {
                cout<<"Thread"<<pthread_self() << " waiting timeout." << endl;
                flag=0;
                break; //超时退出
            }
//            sleep(1);
        }


        (threadpool->idleThreadNum)--;

        //如果任务队列有任务，进行处理
        if(!threadpool->urlQueue.empty())
        {
            string url=threadpool->urlQueue.front();
            cout<<url<<endl;
            threadpool->urlQueue.pop();//取出第一个任务,弹出
            //任务执行时可以解锁，让其他线程操作
            threadpool->cond.unlock();
            BFS(url,(*threadpool));
            cout<<"url的数目："<<threadpool->urlQueue.size()<<endl;
            threadpool->cond.lock();
        }

        //如果是销毁信号，并且任务队列为空
        if(threadpool->isDestroy && threadpool->urlQueue.empty())
        {
            threadpool->nowThreadNum--;
            if(threadpool->nowThreadNum<=0)
                threadpool->cond.signal();
            flag=0;
        }
        threadpool->cond.unlock();
    }

    //退出线程
    cout << "Thread " << pthread_self() << " exited." << endl;
    return arg;
}
void ThreadPool::addUrl(string url) //添加任务
{
    this->cond.lock();
    this->urlQueue.push(url);
    if(this->idleThreadNum>0)//有空闲线程，给等待线程发出唤醒信号
    {
        this->cond.broadcast();
    }
    else//没有空闲线程，看情况添加线程
    {
        if(this->nowThreadNum<this->maxThreadNum)
        {
            pthread_t tid;
            int ret=pthread_create(&tid,nullptr,&threadRoutine,(void *)(this));
            if(ret==0)
            {
                this->threadQueue.push(tid);
                this->nowThreadNum++;
            }
        }
    }
    this->cond.unlock();
}
