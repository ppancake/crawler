//
// Created by zwk on 18-5-28.
//

#ifndef CRAWLER_CONDITION_H
#define CRAWLER_CONDITION_H
#include<pthread.h>
class Condition
{
public:
    Condition(){ init();}
    ~Condition(){destroy();}
    int lock(); //加锁
    int unlock(); //解锁
    int wait(); //等待条件变量
    int timedwait(timespec waittime); //等待一段时间
    int signal(); //给信号
    int broadcast(); //广播信号
    int init(); //初始化互斥锁和条件变量
    int destroy(); //销毁互斥锁和条件变量
private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};


#endif //CRAWLER_CONDITION_H
