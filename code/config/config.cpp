#include "config.h"

config::config()
{
    port_ = 9999 ;          // p  端口
    trigMode_ = 3 ;         // t  触发模式
    timeoutS_ = 10 ;        // m  超时时间 s
    optLinger_ = false ; 

    sqlPort_ = 3306 ;
    sqlUsr_ = "root";
    sqlPSWD_ = "root" ;
    dbName_ = "dbkrain" ;
    sqlPoolNum_ = 6 ;       // s  sql连接池
    threadPoolNum_ = 8 ;    // n  线程池
    openLog_ = true ;       // o  打开日志
    logQueSize_ = 1024 ;    // l  日志队列
}

void config::parse_arg(int argc , char* argv[])
{
    char opt ; 
    const char* str = "p:t:m:o:s:n:l";
    while((opt = getopt(argc , argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port_ = atoi(optarg);  //atoi char to int
            break ;
        }
        case 't':
        {
            trigMode_ = atoi(optarg);
            break ;
        }
        case 'm':
        {
            timeoutS_ = atoi(optarg);
            break ;
        }
        case 'o':
        {
            openLog_ = atoi(optarg);
            break ;
        }
        case 's':
        {
            sqlPoolNum_ = atoi(optarg);
            break ;
        }
        case 'n':
        {
            threadPoolNum_ = atoi(optarg);
            break ;
        }
        case 'l':
        {
            logQueSize_ = atoi(optarg);
            break ;
        }
        
        default:
            break;
        }
    }

}