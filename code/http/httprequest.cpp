#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML_REQUEST{ 
            "/index.html", 
            "/register.html", 
            "/login.html",
            "/welcome.html", 
            "/test.html", 
            "/testVideo.html", 
            "/testMusic.html",
            "/testImage.html",
            };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_POST_REQUEST {
            {"/registerSubmit", 0},
            {"/loginSubmit", 1}
            }; 

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    master_state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
    userName = "";
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && master_state_ != FINISH) {
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);    
        switch(master_state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {   
                return false;
            }
            ParsePath_();  
            break;    
        case HEADERS:
            ParseHeader_(line);   
            if(buff.ReadableBytes() <= 2) { 
                master_state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);  
            break;
        default:
            break;
        }
        if(lineEnd == buff.BeginWrite()) { break; }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_() {
    if(method_ == "GET")  
    {
        char* path_temp ;
        path_temp = (char*) malloc(100);
        if(-1 != sscanf(path_.data(), "%[^?]?%*[^?]", path_temp)) 
        {
            path_ = path_temp ;
        }
        delete path_temp ;
    }

    if(path_ == "/") {
        path_ = "/index.html";
    }
    decode_str(path_ ); 
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        master_state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        master_state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();  
    master_state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

uint32_t HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    if(ch >= '0' && ch <= '9') return ch -'0';
    return ch;
}

void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {   
        if(!ParseBodyForUserInfo()) return ;  
        if(DEFAULT_HTML_POST_REQUEST.count(path_)) {  
            int tag = DEFAULT_HTML_POST_REQUEST.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);

                sqlVerifyState sqlVerifyRes = UserVerify(post_["username"], post_["password"], isLogin);
                switch (sqlVerifyRes)
                {
                case loginOK:
                    /* code */
                    path_ = "/welcome.html";
                    userName = post_["username"] ;
                    break;  
                case registerOK:
                    /* code */
                    path_ = "/login.html";
                    break;
                case loginFail:
                    /* code */
                    path_ = "/loginError.html";
                    break;                
                case registerFail:
                    /* code */
                    path_ = "/registerError.html";
                    break; 
                default:
                    path_ = "/error.html";
                    break;
                }
            }
        }
    }   
}

bool HttpRequest::ParseBodyForUserInfo() {
    if(body_.size() == 0) { return false; }

    char name[100] ;
    char password[100] ;
    //正则表达式匹配，%*[^=] *号表示跳过匹配到的部分，^=表示匹配不是=的内容
    if(-1 != sscanf(body_.data(), "%*[^=]=%[^&]&%*[^=]=%[^=]", name , password))  //匹配等号后面的内容
    {  //body_.data() 或 body_.c_str() 将string转换成 const char*

        decode_str(name);  //解码中文字符
        post_["username"] = name;
        post_["password"] = password ;
        return true ;
    }

    LOG_ERROR("parse username and password error");
    return false;
}

//解码中文字符
void HttpRequest:: decode_str(char *from)
{
    char *to = from ;
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = ConverHex(from[1])*16 + ConverHex(from[2]);
            from += 2;                                  
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}


void HttpRequest:: decode_str(string& from)
{
    unsigned int i , j ;
    for(i = 0, j = 0 ; i < from.size() ; ++ i, ++ j)
    {
        if (from[i] == '%' && isxdigit(from[i+1]) && isxdigit(from[i+2])) { 
            from[j] = ConverHex(from[i+1])*16 + ConverHex(from[i+2]);
            i+= 2;
        }
        else from[j] = from[i];
    }
    for(; j < i ; ++ j) from.pop_back();
}

//判断注册或登陆操作是否有效，判断用户名密码
sqlVerifyState HttpRequest::UserVerify(const std::string& name, const std::string& pwd, bool isLogin){
    if(name == "" || pwd == "") { return otherError; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());  //向sql连接池申请一个连接，析构函数中归还
    assert(sql);
    
    //bool flag = false;
    
    char order[256] = { 0 };
    MYSQL_RES *res = nullptr;
    
    //if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { //查询成功返回0
        mysql_free_result(res);
        return otherError;  //查询失败，内部错误
    }
    res = mysql_store_result(sql);
    //unsigned int j = 0;
    mysql_num_fields(res);
    // MYSQL_FIELD *fields = nullptr;
    mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 登陆行为*/
        if(isLogin) {
            if(pwd == password) { return loginOK; }
            else {
                //flag = false; 
                LOG_DEBUG("pwd error!");
                return loginFail;
            }
        } 
        else {  //注册行为，用户名已被使用
            //flag = false; 
            LOG_DEBUG("user used!");
            return registerFail;
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin) {
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, passwd) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            //flag = false; 
            return otherError;   //mysql插入数据失败，内部错误
        }
        //flag = true;
        return registerOK;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return otherError;;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {   //count 返回key的个数
        return post_.find(key)->second;
    }
    return "";
}