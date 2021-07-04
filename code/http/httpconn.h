#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>  
#include <arpa/inet.h>  
#include <stdlib.h>      
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    void Close();

    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;    
    sockaddr_in GetAddr() const;
    
    bool processData();
    int ToWriteBytes() {  
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {  
        return request_.IsKeepAlive();
    }

    static bool isET;   //边沿触发
    static const char* srcDir;  
    static std::atomic<int> userCount;   
    
private:
    int fd_;
    struct  sockaddr_in addr_;
    bool isClose_;
    int iovCnt_;
    struct iovec iov_[2];  //聚集写
    Buffer readBuff_;	 // 读缓冲
    Buffer writeBuff_;	 // 写缓冲
    HttpRequest request_;  
    HttpResponse response_;   
};

#endif