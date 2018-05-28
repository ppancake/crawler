//
// Created by zwk on 18-5-28.
//

#include "crawlerImg.h"
#include <iostream>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <error.h>
#include <sys/stat.h>
#include <vector>
#include <ext/hash_set>
#include <unordered_set>


int ncount=1;

//通过url得主机及资源路径
bool parseUrl(const string &url, string &host,string &resource)
{
    size_t pos_protocal=url.find("http://");
    if(pos_protocal==string::npos)
        return false;
    pos_protocal+=strlen("http://");
    size_t pos_sitename=url.find_first_of('/',pos_protocal);
    host=url.substr(pos_protocal,pos_sitename-pos_protocal);
    resource=url.substr(pos_sitename,url.size()-pos_sitename);
    return true;
}

//获取html
bool getHttpResponse(const string &host,const string &resource,string &response,int &nret)
{
    struct hostent *hostIp=gethostbyname(host.c_str());
    int sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock<0)
    {
        cout<<"创建socket失败"<<endl;
        return false;
    }
    //设置超时
    int timeOut=1000;
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeOut,sizeof(int));

    sockaddr_in srvaddr;
    srvaddr.sin_family=AF_INET;
    srvaddr.sin_port=htons(80);
//    srvaddr.sin_addr.s_addr=inet_addr(inet_ntoa(*(in_addr *)hostIp->h_addr_list[0]));
    memcpy(&srvaddr.sin_addr, hostIp->h_addr, 4);
    // srvaddr.sin_addr=*((struct in_addr *)hostIp->h_addr_list);  //强行转换
    // char *ip=inet_ntoa(srvaddr.sin_addr);

    //连接服务器
    if((connect(sock,(struct sockaddr*)&srvaddr,sizeof(srvaddr)))<0 )
    {
        cout<<"socket连接失败"<<endl;
        return false;
    }

    //发送数据，获取格式
    //   string request="GET"+resource+"HTTP/1.1\r\nHost:"+host+"\r\nConnection:Close\r\n\r\n";
    string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\n\r\n";
    if(send(sock,request.c_str(),request.size(),0)<0)
    {
        cout<<"发送数据失败"<<endl;
        return false;
    }

    //接受数据
    int htmlSize=1048576;
    char *recvbuf=new char[htmlSize];
    memset(recvbuf,0,htmlSize);
    int ret=1;
    int recvLen=0;
    cout<<"读取字节数:"<<endl;
    while(ret>0)
    {
        ret=recv(sock,recvbuf+recvLen,htmlSize-recvLen,0);
        if(ret>0)
            recvLen+=ret;
        if(htmlSize-recvLen<100)  //空间不足，加倍
        {
            cout<<"重新分配空间"<<endl;
            char *newbuf=new char[2*htmlSize];
            strcpy(newbuf,recvbuf);
            delete[] recvbuf;
            recvbuf=newbuf;
        }
        cout<<recvLen<<"  ";
    }
    recvbuf[recvLen]='\0';
    response.assign(recvbuf,recvLen);
    nret=recvLen;
    delete[] recvbuf;


    close(sock);
    return true;
}

//url名字转文件名
string ToFilename(const string url)
{
    string filename;
    filename.resize(url.size());
    int i=0;
    for(int j=0;j<url.size();j++) {
        char ch = url[j];
        if (ch != '\\' && ch != '/' && ch != ':' && ch != '*' && ch != '?' && ch != '"' && ch != '<' && ch != '>' &&
            ch != '|')
        {
            filename[i++] = ch;
        }

    }
    return filename.substr(0,i)+".txt";
}

//解析html，得到url
bool parseHtml(const string &response,vector<string> &vecImg,ThreadPool &threadpool)
{
    //解析url
    string head="href=\"http://";
    size_t pos_head=response.find(head);
    ofstream ofile("url.txt",ios::app); //以追加方式打开

    //查找打url
    while (pos_head!=string::npos)
    {
        pos_head+=strlen("href=\"");
        size_t pos_url=response.find('"',pos_head);
        string tmpurl=response.substr(pos_head,pos_url-pos_head);
        //url未访问过
        if(threadpool.visitedUrl.find(tmpurl)==threadpool.visitedUrl.end())
        {
            threadpool.visitedUrl.insert(tmpurl);

            if(threadpool.visitedUrl.size()>100000)
                threadpool.visitedUrl.clear();        //hashset多大可以去清除呢
            ofile<<tmpurl<<endl;
            threadpool.addUrl(tmpurl);
        }

        pos_head=response.find(head,pos_url);
    }
    ofile.close();

    //解析图片url
    string img = "http://";
    size_t pos_img_head = response.find(img);
    while (pos_img_head != string::npos) {
        size_t pos_img = response.find('"', pos_img_head + 1);
        if (pos_img == string::npos)
            return true;
        string imgurl = response.substr(pos_img_head, pos_img - pos_img_head);
        size_t pos_point = imgurl.find_last_of('.');
        if (pos_point == string::npos)
            return true;
        string ext = imgurl.substr(pos_point + 1, imgurl.size() - pos_point - 1);
        if (ext.compare("jpg") && ext.compare("jpeg") && ext.compare("png") && ext.compare("gif") && ext.compare("bmp")) {
            pos_img_head = response.find(img, pos_img + imgurl.size());
            continue;
        }
        if (threadpool.visitedImg.find(imgurl) == threadpool.visitedImg.end()) {
            threadpool.visitedImg.insert(threadpool.visitedImg.end(), imgurl);
            if (threadpool.visitedImg.size()>100000)
                threadpool.visitedImg.clear();
            vecImg.push_back(imgurl);
        }
        pos_img_head = response.find(img, pos_img + imgurl.size());
    }
    cout<<"结束这个页面的解析"<<endl;
    return true;
}

//下载
void downloadImg(const vector<string> &vecImg)
{

    string filename="./img";
    for(int i=0;i<vecImg.size();i++)
    {
        string imgUrl=vecImg[i];
        cout<<imgUrl<<endl;
        size_t  pos_point=imgUrl.find_last_of('.');
        string imgType=vecImg[i].substr(pos_point+1,imgUrl.size()-pos_point-1);
        if(imgType.compare("jpg") && imgType.compare("jpeg") && imgType.compare("png") &&imgType.compare("gif") &&imgType.compare("bmp"))
            continue;

        //对图片URL解析
        string host;
        string resource;
        if(!parseUrl(imgUrl,host,resource))
        {
            cout<<"网址解析出错"<<endl;
            return;
        }
        string img;
        int nret;
        if(getHttpResponse(host,resource,img,nret))
        {
            if(nret==0)
            {
                cout<<"传或数据有问题"<<endl;
                continue;
            }
            size_t pos_end=img.find("\r\n\r\n");
            if(pos_end==string::npos)
            {
                cout<<"数据有问题，结尾有问题"<<endl;
                continue;
            }
            pos_end+=strlen("\r\n\r\n");
            if(pos_end==nret)
            {
                cout<<"返回数据一下就到低了"<<endl;
                continue;
            }
            size_t pos=imgUrl.find_last_of("/");
            if(pos!=string::npos)
            {
                string imgname=imgUrl.substr(pos,imgUrl.size()-pos);
                ofstream ofile(filename+imgname,ios::binary);
                if(!ofile.is_open())
                    continue;
                cout<<ncount++<<filename+imgname<<endl;
                ofile.write(&img[pos_end],nret-pos_end-1);
                ofile.close();
            }
        }
    }

}

//广度优先解析种子
void BFS(string url,ThreadPool& threadpool)
{
    string host;
    string resource;
    //解析url
    if(!parseUrl(url,host,resource))
    {
        cout<<"url解析出错"<<endl;
        return ;
    }

    //得到返回值
    string response;
    int nret=0;
    if(!getHttpResponse(host,resource,response,nret))
    {
        cout<<"获取网页失败"<<endl;
        return;
    }

    //保存页面的名字并打开
    string filename=ToFilename(url);
    ofstream ofile("./html/"+filename);
    if(!ofile.is_open())
    {
        cout<<"打开文件出错"<<endl;
        return;
    }
    ofile<<response<<endl;
    ofile.close();
    cout<<filename<<endl;
    vector<string> vecImg;

    if(parseHtml(response,vecImg,threadpool)==0)
    {
        cout<<"解析网页失败"<<endl;
        return;
    }

    //将vecImg的图片依次下载
    cout << "Thread " << pthread_self() << " downloading" << endl;
    downloadImg(vecImg);


}