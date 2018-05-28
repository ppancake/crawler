//
// Created by zwk on 18-5-28.
//

#ifndef CRAWLER_CRAWLERIMG_H
#define CRAWLER_CRAWLERIMG_H

#include "ThreadPool.h"
#include <string>
#include <vector>
using namespace std;
using namespace __gnu_cxx;


//根据url得到网站信息
bool parseUrl(const string &url, string &host,string &resource);

//获取html
bool getHttpResponse(const string &host,const string &resource,string &response,int &nret);

//url名字转文件名
string ToFilename(const string url);

//解析html，得到url
bool parseHtml(const string &response,vector<string> &vecImg,ThreadPool &threadpool);

//下载
void downloadImg(const vector<string> &vecImg);

//广度优先解析种子
void BFS(string url,ThreadPool& threadpool);



#endif //CRAWLER_CRAWLERIMG_H
