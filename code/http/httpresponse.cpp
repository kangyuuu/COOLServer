#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<string, string> HttpResponse::SOURCE_FOLDER = {
    { ".html",  "/html" },
    { ".xml",   "/file" },
    { ".xhtml", "/html" },
    { ".txt",   "/file" },
    { ".rtf",   "/file" },
    { ".pdf",   "/file" },
    { ".word",  "/file" },
    { ".png",   "/image" },
    { ".gif",   "/image" },
    { ".ico",   "/html" },
    { ".jpg",   "/image" },
    { ".jpeg",  "/image" },
    { ".au",    "/music" },
    { ".mpeg",  "/video" },
    { ".mpg",   "/video" },
    { ".avi",   "/video" },
    { ".gz",    "/file" },
    { ".tar",   "/file" },
    { ".css",   "/file"},
    { ".js",    "/file"},
    { ".flac",  "/music"},
    { ".lrc",   "/music"},
    { ".mp4",   "/video"},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::ERR_CODE_PATH = {
    { 400, "/400.html" },   //请求无效
    { 403, "/403.html" },   //禁止访问
    { 404, "/404.html" },   //文件不存在
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const string& srcDir, string& path, bool isKeepAlive, int code){
    assert(srcDir != "");
    if(mmFile_) { UnmapFile(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& buff) {
    //定位到对应的文件夹
    char fileSuffix[10] ;
    *fileSuffix = '.';
    if(-1 != sscanf(path_.data(), "%*[^.].%[^.]", fileSuffix+1 )) 
    {
        if(SOURCE_FOLDER.find(fileSuffix) == SOURCE_FOLDER.end()) code_ = 404;
        else path_ = SOURCE_FOLDER.find(fileSuffix)->second + path_ ; 
        MakeResponse_FILE(buff); 
    }
    else 
    {
        MakeResponse_MENU(buff);
    }
    return ;
}

void HttpResponse::MakeResponse_FILE(Buffer& buff)  
{
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    ErrorHtml_();  
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}
void HttpResponse::MakeResponse_MENU(Buffer& buff) 
{
    if(path_ == "/MENU") path_.clear();

    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISREG(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }

    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddMenuHTML(buff);

    return ;
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

void HttpResponse::ErrorHtml_() {
    if(ERR_CODE_PATH.count(code_) == 1) {
        path_ = ERR_CODE_PATH.find(code_)->second;  
        stat((srcDir_ + path_).data(), &mmFileStat_);  
    }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");
        return; 
    }

    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::AddMenuHTML(Buffer &buff) 
{
    string body ;

    body.clear();
    body += "<!DOCTYPE html>";
    body += "<html><head><meta charset=\"UTF-8\"><title>EXPLORE</title><head>";
    body += "<body><h3>当前目录："+ path_ + "</h3><table>" ;

    struct dirent** dirPtr ;
    int res = scandir((srcDir_ + path_).data() , &dirPtr , NULL , alphasort); //扫描文件夹

    struct stat tempStat ;
    for(int i = 0 ; i < res ; ++ i)
    {
        string name = dirPtr[i]->d_name;

        if(name == "." || name == "..") continue ;
        
        stat((srcDir_+path_+ "/"+name).data() , &tempStat);

        if(S_ISREG(tempStat.st_mode)) 
        {
            body += "<tr><td><a href=\"" ;
            body += name ;
            body += "\">";
            body += name ;
            body +=  "</a></td><td>";
            body +=  to_string((float)tempStat.st_size/1024);
            body += "Mb</td></tr>";
        }
        else if(S_ISDIR(tempStat.st_mode))  
        {
            body += "<tr><td><a href=\"" ;
            if(!path_.empty()) body+= path_ + "/";
            body += name ;
            body += "\">";
            body += name ;
            body +=  "</a></td><td>";
            body +=  to_string((float)tempStat.st_size/1024);
            body += "Mb</td></tr>";
        }
    }

    body += "</table></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

void HttpResponse::encode_str(std::string& from)
{
	std::string strTemp = "";
	size_t length = from.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)from[i]) ||
            (from[i] == '/') ||
			(from[i] == '-') ||
			(from[i] == '_') ||
			(from[i] == '.') ||
			(from[i] == '~'))
			strTemp += from[i];
		else if (from[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)from[i] >> 4);
			strTemp += ToHex((unsigned char)from[i] % 16);
		}
	}
    from = strTemp;
    cout << "from : " << from << endl ;
    cout << "strTemp : " << strTemp << endl ;
	return ;
}

unsigned char HttpResponse::ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

string HttpResponse::GetFileType_() {
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) { 
        return "text/html";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>COOLWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
