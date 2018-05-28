//
// Created by zwk on 18-5-28.
//

#ifndef CRAWLER_THREADPOOL_H
#define CRAWLER_THREADPOOL_H
#include<pthread.h>
#include<string>
#include<queue>
#include<set>
#include <unordered_set>
#include "Condition.h"
using std::string;
using std::queue;
using std::set;
using std::unordered_set;



class ThreadPool
{
public:
    void initThreadPool();//初始化线程池
    void destroyThreadPool();//销毁线程池
    friend void *threadRoutine(void* arg); //线程入口函数
    friend void BFS(string url,ThreadPool&);
    void addUrl(string url);//添加任务??之前放到析构函数后面，：：居然不显示？？
    ThreadPool(string url,int initNum, int maxNum):seedUrl(url),initThreadNum(initNum),nowThreadNum(0),idleThreadNum(0),maxThreadNum(maxNum),isDestroy(false)
    {
        initThreadPool();
    }
    ~ThreadPool()
    {
        destroyThreadPool();
    }

public:
    queue<string> urlQueue;  //url队列
    unordered_set<string> visitedUrl;  //已访问的url
    unordered_set<string> visitedImg;  //已访问的图片
private:
    string seedUrl;//初始url
    Condition cond; //锁和条件变量
    int initThreadNum; //初始线程池数量
    int nowThreadNum; //当前线程池线程数量
    int idleThreadNum; //当前空闲线程数量
    int maxThreadNum; //最大允许的线程数量
    bool isDestroy; //线程销毁通知
    queue<pthread_t> threadQueue;

};


#endif //CRAWLER_THREADPOOL_H
