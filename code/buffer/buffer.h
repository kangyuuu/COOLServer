#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>  
#include <iostream>
#include <unistd.h>  
#include <sys/uio.h> 
#include <vector> 
#include <atomic>
#include <assert.h>
class Buffer {
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;
    size_t WritableBytes() const;       //可写的字节       
    size_t ReadableBytes() const ;      //可读的字节
    size_t PrependableBytes() const;    //前置字节

    const char* Peek() const;      //可读数据的起始位置
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);

    void Recycle(size_t len);     //回收缓冲区
    void RecycleTo(const char* end);
    void RecycleFrom(const char* from) ;
    void RecycleAll();     		  //全部回收
    std::string RecycleAllReturnStr();   

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);    //追加数据
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);   //读fd
    ssize_t WriteFd(int fd, int* Errno);  //写fd

private:
    char* BeginPtr_();              
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);   //整理缓冲区

    std::vector<char> buffer_;          //真实数据
    std::atomic<std::size_t> readPos_;  //读起始位置
    std::atomic<std::size_t> writePos_; //写起始位置
};

#endif