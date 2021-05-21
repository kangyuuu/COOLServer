#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

struct TimerNode {
    int fd_;                                
    timeval endTime_;                        //时间戳                                                  
    std::function<void()> cb_;              //超时回调 function<void()>
    TimerNode* next ;
    TimerNode* pre ;
    TimerNode(): fd_(-1), next(nullptr), pre(nullptr){} 
    bool operator<(const TimerNode& t) {  
        return endTime_.tv_sec < t.endTime_.tv_sec;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer() ;
    
    void updateNode(int fd, int timeout);      
    void addNode(int fd, int timeout, const std::function<void()> cb); 
    int nextNodeClock();     

private:
    void removeNode(int fd);  
    void removeEnding();    
    void nodeToBack(TimerNode* node);   


    TimerNode* head_ ;  
    TimerNode* back_ ;  
    std::unordered_map<int, TimerNode*> node_map_;  
};

#endif //HEAP_TIMER_H