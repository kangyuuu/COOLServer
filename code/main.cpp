#include "config/config.h"
#include "server/webserver.h"

int main(int argc, char** argv) {
    //daemon(1, 0);   //脱离控制台

    config* CFG = new config();
    if(argc > 1) CFG->parse_arg(argc , argv);  //解析参数
    WebServer server(CFG);  

    server.Launch();

    return 0 ;
} 
  