#include "config/config.h"
#include "server/webserver.h"

int main(int argc, char** argv) {
    //daemon(1, 0);   //以守护进程的形式后台运行

    config* CFG = new config();
    if(argc > 1) CFG->parse_arg(argc , argv); 
    WebServer server(CFG);  

    server.Launch();

    return 0 ;
} 
  