#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

enum sqlVerifyState { 
    loginOK = 0 , 
    registerOK , 
    loginFail , 
    registerFail, 
    otherError
};

class HttpRequest {
public:
    enum PARSE_STATE {  //主状态机
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    bool parse(Buffer& buff);  

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;

private:

    std::string userName ;

    bool ParseRequestLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);

    void ParsePath_();
    void ParsePost_();
    bool ParseBodyForUserInfo();

    static sqlVerifyState UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE master_state_;   
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_; 
    std::unordered_map<std::string, std::string> post_;  

    static const std::unordered_set<std::string> DEFAULT_HTML_REQUEST;           
    static const std::unordered_map<std::string, int> DEFAULT_HTML_POST_REQUEST; 
    static uint32_t ConverHex(char ch); 
    void decode_str(char *from);       
    void decode_str(std::string& from);
};


#endif //HTTP_REQUEST_H