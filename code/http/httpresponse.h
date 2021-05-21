#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       
#include <unistd.h>      
#include <sys/stat.h>    
#include <sys/mman.h>   
#include <dirent.h>     

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void MakeResponse_FILE(Buffer& buff);   
    void MakeResponse_MENU(Buffer& buff);  
    void UnmapFile();

    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message); 
    int Code() const { return code_; }

private:
    void AddStateLine_(Buffer &buff);   
    void AddHeader_(Buffer &buff);     
    void AddContent_(Buffer &buff);    
    void AddMenuHTML(Buffer &buff);   
    void encode_str(std::string& from); 
    unsigned char ToHex(unsigned char x);

    void ErrorHtml_();  
    std::string GetFileType_();

    int code_;   
    bool isKeepAlive_;

    std::string path_;    
    std::string srcDir_;  
    
    char* mmFile_;             
    struct stat mmFileStat_;    

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;   
    static const std::unordered_map<std::string, std::string> SOURCE_FOLDER; 
    static const std::unordered_map<int, std::string> CODE_STATUS;         
    static const std::unordered_map<int, std::string> ERR_CODE_PATH;      
};


#endif //HTTP_RESPONSE_H