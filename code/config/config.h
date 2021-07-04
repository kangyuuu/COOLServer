#pragma once

#include <string>
#include <stdlib.h>
#include <unistd.h>

class config
{
public:
    config();
    ~config() = default ;

    void parse_arg(int argc , char* argv[]);

    int port_ ;         
    int trigMode_ ;   
    int timeoutS_ ;   
    bool optLinger_ ;  

    int sqlPort_ ;    
    std::string sqlUsr_ ;
    std::string sqlPSWD_ ;
    std::string dbName_ ;
    int sqlPoolNum_ ;

    int threadPoolNum_ ;  
    bool openLog_ ;      
    int logQueSize_ ;   

};