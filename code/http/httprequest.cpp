#include "httprequest.h"
#include <fstream>
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML_REQUEST{  //页面信息
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
            {"/loginSubmit", 1},
            {"/uploadfile", 2}
            };

void HttpRequest::Init() {
    method_ = path_ = version_ = "";
    body_ = nullptr ;
    master_state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
    userName = "";

    fileSaveDir_ = getcwd(nullptr, 256);
    assert(fileSaveDir_.data());
    fileSaveDir_ += "/../resources/upload";
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parseData(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && master_state_ != FINISH) {
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);//在buff中匹配\r\n
        std::string line(buff.Peek(), lineEnd);   
        switch(master_state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {   //解析请求行
                return false;
            }
            ParsePath_(); 
            buff.RecycleTo(lineEnd + 2);
            break;    
        case HEADERS:
            ParseHeader_(line);    			//解析请求头
            if(buff.ReadableBytes() <= 2) {
                master_state_ = FINISH;
            }
            buff.RecycleTo(lineEnd + 2);
            break;
        case BODY:
            body_ = &buff;
            ParseBody_(); 					//解析请求体
            break;
        default:
            break;
        }
    }
    buff.RecycleAll();
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_() {
    if(method_ == "GET")
    {
        char* path_temp ;
        path_temp = (char*) malloc(100);
        if(-1 != sscanf(path_.data(), "%[^?]?%*[^?]", path_temp))  //匹配
        {
            path_ = path_temp ;
        }
        delete path_temp ;
    }

    if(path_ == "/") {
        path_ = "/index.html";
    }
    decode_str(path_ );  //解码中文字符
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

void HttpRequest::ParseBody_() {
    ParsePost_();   
    master_state_ = FINISH;
}

uint32_t HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    if(ch >= '0' && ch <= '9') return ch -'0';
    return ch;
}

void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") 
    {   
        if(!ParseBodyForUserInfo()) return ;  
        if(DEFAULT_HTML_POST_REQUEST.count(path_)) {  //验证，注册或登陆
            int tag = DEFAULT_HTML_POST_REQUEST.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                sqlVerifyState sqlVerifyRes = UserVerify(post_["username"], post_["password"], isLogin);
                switch (sqlVerifyRes)
                {
                case loginOK:
                    path_ = "/welcome.html";
                    userName = post_["username"] ;
                    break;  
                case registerOK:
                    path_ = "/login.html";
                    break;
                case loginFail:
                    path_ = "/loginError.html";
                    break;                
                case registerFail:
                    path_ = "/registerError.html";
                    break; 
                default:
                    path_ = "/error.html";
                    break;
                }
            }
        }
    } 
    else if(method_ == "POST" && header_["Content-Type"].find("multipart/form-data") != string::npos) //对上传的文件进行解析
    {
        if(DEFAULT_HTML_POST_REQUEST.count(path_) && ParseBodyForFile()) path_ = "upload_ok";   
        else path_ = "upload_err";
    } 

    master_state_ = FINISH;
}

bool HttpRequest::ParseBodyForFile() {
    if(body_->ReadableBytes() <= 0) return false;
    char fileName[256];
    const char CRLF[] = "\r\n";
    const char* lineEnd;

    while(true)
    {
        lineEnd = search(body_->Peek(), body_->BeginWriteConst(), CRLF, CRLF + 2);
        if(lineEnd == body_->Peek())  
        {
            break ;
        }
        std::string line(body_->Peek(), lineEnd);     
        if(line.find("filename") != string::npos)   
        {   
            char targ[10] = "filename";
            const char* temp = search(line.data() , line.data()+line.size() , targ , targ + 8);
            if(-1 != sscanf(temp, "%*[^=]=\"%[^=^\"]", fileName)) 
            {
                decode_str(fileName); 
            }         
        }
        body_->RecycleTo(lineEnd + 2);
    }

    char targ[10] = "------";     //buff最后一行是分隔符的形式
    const char* temp = search(body_->Peek() , body_->Peek() + body_->ReadableBytes() , targ , targ + 6);
    if(temp) body_->RecycleFrom(temp); 

    assert(body_->Peek() == lineEnd);

    return SaveFile(fileName) ;
}

bool HttpRequest::ParseBodyForUserInfo() {
    if(body_->ReadableBytes() <= 0) { return false; }

    char name[100] ;
    char password[100] ;
    if(-1 != sscanf(body_->Peek(), "%*[^=]=%[^&]&%*[^=]=%[^=]", name , password)) 
    {
        decode_str(name);  //解码中文字符
        post_["username"] = name;
        post_["password"] = password ;
        return true ;
    }
    LOG_ERROR("parse username and password error");
    return false;
}

bool HttpRequest::SaveFile(char filename[]) 
{
    if(fileSaveDir_.empty()) LOG_DEBUG("httprequest: fileSaveDir_ = s%" , fileSaveDir_);

    ofstream FILE_save ;
    FILE_save.open(fileSaveDir_ + "/" + filename);
    if(FILE_save.is_open()) 
    {
        FILE_save<<body_->Peek();
        LOG_INFO("httprequest: save file successed ! dir : %s" , fileSaveDir_ + "/" + filename);
    }
    else 
    {
        LOG_DEBUG("httprequest: save file error , can not open file !");
    }
    FILE_save.close();

    return true ;
}

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

sqlVerifyState HttpRequest::UserVerify(const std::string& name, const std::string& pwd, bool isLogin){
    if(name == "" || pwd == "") { return otherError; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance()); 
    assert(sql);
    
    char order[256] = { 0 };
    MYSQL_RES *res = nullptr;
    
    snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return otherError;  
    }
    res = mysql_store_result(sql);
    mysql_num_fields(res);
    mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        if(isLogin) {
            if(pwd == password) { return loginOK; }
            else {
                LOG_DEBUG("pwd error!");
                return loginFail;
            }
        } 
        else { 
            LOG_DEBUG("user used!");
            return registerFail;
        }
    }
    mysql_free_result(res);

    if(!isLogin) {
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, passwd) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!"); 
            return otherError; 
        }
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
    if(post_.count(key) == 1) { 
        return post_.find(key)->second;
    }
    return "";
}