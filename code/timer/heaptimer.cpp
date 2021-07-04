#include "heaptimer.h"

HeapTimer::HeapTimer()
{
    head_ = new TimerNode() ;
    back_ = new TimerNode() ;
    head_->fd_ = -1 ;
    back_->fd_ = -1 ;
    head_->next = back_ ;
    back_->pre = head_ ;
    node_map_.clear() ;
}

HeapTimer::~HeapTimer() {
    unordered_map<int,TimerNode*>::iterator iter = node_map_.begin() ;
    while(iter != node_map_.end()) removeNode(iter->first) ;
    delete head_ ;
    delete back_ ;
    head_ = nullptr ;
    back_ = nullptr ;    
    node_map_.clear();
}

void HeapTimer::nodeToBack(TimerNode* node)   {
    assert(node != nullptr );

    node->pre->next = node->next ;
    node->next->pre = node->pre ;

    node->pre = back_->pre ;
    node->pre->next = node ;
    node->next = back_ ;
    back_->pre = node ;
}

void HeapTimer::addNode(int fd, int timeout, const std::function<void()> cb) {
    assert(fd >= 0);
    if(node_map_.find(fd) == node_map_.end()) {
        TimerNode* temp = new TimerNode();
        temp->fd_ = fd ;
        gettimeofday(&temp->endTime_ , nullptr);
        temp->endTime_.tv_sec += timeout ;
        temp->cb_ = cb ;

        temp->pre = back_->pre ;
        temp->pre->next = temp ;
        temp->next = back_ ;
        back_->pre = temp ;

        node_map_[fd] = temp ;
    } 
    else updateNode(fd , timeout);
}

void HeapTimer::removeNode(int fd) {

    if(node_map_.find(fd) == node_map_.end()) return ;
    TimerNode* node = node_map_[fd] ;
    assert(node != nullptr);  
     
    node->pre->next = node->next ;
    node->next->pre = node->pre ;
    node_map_.erase(node->fd_);

    node->cb_(); 
    delete node ;
}

void HeapTimer::updateNode(int fd, int timeout) {
    assert(node_map_.find(fd) != node_map_.end());
    gettimeofday(&node_map_[fd]->endTime_, nullptr);
    node_map_[fd]->endTime_.tv_sec += timeout ;   
    nodeToBack(node_map_[fd]);  
}

void HeapTimer::removeEnding() {
    if(node_map_.empty()) return ;
    TimerNode* node = head_->next ;
    timeval tmv ;
    while(node != back_) {
        gettimeofday(&tmv , nullptr);  
        if(tmv.tv_sec < node->endTime_.tv_sec) break ;
        if(node_map_.find(node->fd_) == node_map_.end()) 
        {
            LOG_ERROR("fd : %d timer error" , node->fd_);
            continue ;
        }
        removeNode(node->fd_);
        node = head_->next ;      
    }
}

int HeapTimer::nextNodeClock() {
    removeEnding();    //清除超时节点
    if(node_map_.empty()) return -1 ;
    timeval tmv ;
    gettimeofday(&tmv , nullptr);
    return head_->next->endTime_.tv_sec - tmv.tv_sec ;
}