
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "ThreadPool.h"
#include "crawlerImg.h"


using namespace std;


const char  *filename="test.txt";



int main() {
    int create=mkdir("./html",S_IRWXU)&&mkdir("./img",S_IRWXU);
    if(create<0)
    {
        cout<<"创建文件夹失败"<<endl;
        return 1;
    }
    ifstream  file("hrefUrl.txt");
    //file.open(filename,ofstream::out);

    string seedUrl;
    if(!file.is_open())
    {
        cout<<"文件打开错误"<<endl;
        return 1;
    }
    getline(file,seedUrl);
    file.close();
    ThreadPool threadpool(seedUrl,1,5);

    return 0;
 }