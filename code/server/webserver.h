#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>      
#include <unistd.h>     
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"
#include "../config/config.h"

class WebServer {
public:
    WebServer(config* cfgObj);
    ~WebServer();
    void Launch();

private:
    bool InitSocket_lfd();   
    void InitEventMode_(int trigMode);  
    void AddClient_(int fd, sockaddr_in addr);  
  
    void DealListen_();  
    void DealWrite_(HttpConn* client);   
    void DealRead_(HttpConn* client);   

    void SendError_(int fd, const char*info);   
    void ExtentTime_(HttpConn* client);  //更新定时器状态 
    void CloseConn_(HttpConn* client);   //关闭连接

    void OnRead_(HttpConn* client);   //从客户端读
    void OnWrite_(HttpConn* client);    //向客户端写
    void OnProcess(HttpConn* client);    //解析处理

    static const int MAX_FD = 65536;  
    static int SetFdNonblock(int fd);  

    int port_;
    bool openLinger_;
    int timeoutS_; 
    bool isClose_;
    int listenFd_;
    char* srcDir_;
    
    uint32_t listenEvent_;  
    uint32_t connEvent_;
   
    std::unique_ptr<HeapTimer> timer_;          //定时器
    std::unique_ptr<ThreadPool> threadpool_;    //线程池
    std::unique_ptr<Epoller> epoller_;          //epoll监听树
    std::unordered_map<int, HttpConn> users_;   //维护所有的http连接
};


#endif 