#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <queue>
#include <thread>
#include <stdlib.h>    
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           
#include <assert.h>
#include <sys/stat.h>       
#include "../buffer/buffer.h"

using namespace std ;

enum LOG_LEVEL { V_INFO = 0 , V_DEBUG , V_WARN , V_ERROR}; 

class Log 
{
public :
    void init( const char* path = "./../log", 
                const char* suffix = ".log" ) ;
    static Log* Instance() ;  //单例化模式
    static void flushLogThreadRun() ;  

    void asyncWriteLog() ;     
    void logAdd(LOG_LEVEL level, const char *format,...); //添加一条日志
    void AppendLogLevelTitle_(LOG_LEVEL level) ;

    bool IsOpen() ;

private:
    Log();  
    virtual ~Log();

    int lineCnt_ ;
    bool isAsync_ ;
    bool isOpen_ ;

    Buffer buff_ ;
    int today_ ;

    FILE* fileptr_ ;
    mutex logmtx_;
    condition_variable que_not_empty;

    unique_ptr<thread>  workThread_ ; 
    queue<string> logQue_ ;     
    
    const char* path_ ;    
    const char* suffix_ ;   

    static const int LOG_NAME_LEN = 100 ;
    static const int MAX_LINES = 5000 ;

};
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen()) {\
            log->logAdd(level, format, ##__VA_ARGS__); \
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do{ LOG_BASE(V_DEBUG, format, ##__VA_ARGS__) } while(0);
#define LOG_INFO(format, ...) do{ LOG_BASE(V_INFO, format, ##__VA_ARGS__) } while(0);
#define LOG_WARN(format, ...)  do{ LOG_BASE(V_WARN, format, ##__VA_ARGS__) } while(0);
#define LOG_ERROR(format, ...)  do{ LOG_BASE(V_ERROR, format, ##__VA_ARGS__) } while(0);

#endif 
