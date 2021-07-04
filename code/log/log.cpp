#include "log.h"

Log::Log() { 

    lineCnt_ = 0;
    isAsync_ = true;
    isOpen_ = false ;
    today_ = 0;

    fileptr_ = nullptr;
    workThread_ = nullptr;

}

Log::~Log() {
    if(workThread_ && workThread_->joinable()) {
        while(!logQue_.empty()) {
            que_not_empty.notify_all();
        };
        workThread_->join();
    }
    if(fileptr_) {
        lock_guard<mutex> locker(logmtx_);
        fflush(fileptr_);   //刷新输出缓冲区
        fclose(fileptr_);
    }
}

void Log::init(const char* path, const char* suffix )
{
    path_ = path ;
    suffix_ = suffix ;
    isOpen_ = true ;
    lineCnt_ = 0 ;

    if(workThread_ == nullptr)
    {
        unique_ptr<thread> newThread(new thread(flushLogThreadRun));
        workThread_ = move(newThread);
    }

    time_t timeNow = time(nullptr);  
    struct tm *sysTime = localtime(&timeNow);
    today_ = sysTime->tm_mday;

    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, suffix_);

    lock_guard<mutex> locker(logmtx_);
    buff_.RecycleAll();
    if(fileptr_) { 
            if(!logQue_.empty()) que_not_empty.notify_all();
            fflush(fileptr_); 
            fclose(fileptr_); 
        }
    fileptr_ = fopen(fileName, "a"); 

    if(fileptr_ == nullptr) {
        mkdir(path_, 0777);
        fileptr_ = fopen(fileName, "a");
    } 
    assert(fileptr_ != nullptr);
                    
}

Log* Log::Instance() 
{
    static Log logObject ;
    return &logObject ;
}

void Log::flushLogThreadRun() 
{
    Log::Instance()->asyncWriteLog();
}

void Log::asyncWriteLog()
{
    while (true)  
    {
        unique_lock<mutex> locker(logmtx_);
        if(logQue_.empty()) 
        {
            que_not_empty.wait(locker);
        }
        
        fputs(logQue_.front().c_str(), fileptr_);
        fflush(fileptr_);   
        logQue_.pop();
    }    
}

bool Log::IsOpen()
{
    lock_guard<mutex> locker(logmtx_);
    return isOpen_ ;
}

void Log::AppendLogLevelTitle_(LOG_LEVEL level) {
    switch(level) {
    case V_DEBUG:
        buff_.Append("[debug]: ", 9);
        break;
    case V_INFO:
        buff_.Append("[info] : ", 9);
        break;
    case V_WARN:
        buff_.Append("[warn] : ", 9);
        break;
    case V_ERROR:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::logAdd(LOG_LEVEL level, const char *format,...)
{
    struct timeval now = {0 ,0}; 
    gettimeofday(&now , nullptr);
    time_t timer = time(nullptr);    
    struct tm* sysTime = localtime(&timer);

    if(today_ != sysTime->tm_mday || (lineCnt_ && (lineCnt_ % MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(logmtx_);
        locker.unlock() ;

        char newFile[LOG_NAME_LEN];
        char date[36] = {0};
        snprintf(date , 36 , "%04d_%02d_%02d", sysTime->tm_yday + 1900  , sysTime->tm_mon + 1 , sysTime->tm_mday);

        if (today_ != sysTime->tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, date, suffix_);
            today_ = sysTime->tm_mday;
            lineCnt_ = 0;
        }
        else {   //log文件超maxline行数  加 -num 后缀
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, date, (lineCnt_  / MAX_LINES), suffix_);
        }

        locker.lock();
        fflush(fileptr_);
        fclose(fileptr_);

        fileptr_ = fopen(newFile, "a");
        assert(fileptr_ != nullptr);
    }

    unique_lock<mutex> locker(logmtx_);
    lineCnt_ ++ ;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday,
                    sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec, now.tv_usec);

    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    va_list vaList ;
    va_start(vaList, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
    va_end(vaList);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if(isAsync_)
    {
        logQue_.push(buff_.RecycleAllReturnStr());
        que_not_empty.notify_one();  //唤醒一个线程
    }
    else fputs(buff_.Peek(), fileptr_);

    buff_.RecycleAll();
}