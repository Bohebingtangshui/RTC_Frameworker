#pragma once
#include <atomic>
namespace xrtc
{
    //one producer and one consumer
    //pointer 
    template <typename T>
    class LockFreeQueue
    {
    private:
        struct Node
        {
            T data;
            Node* next;
            Node(const T& t):data(t),next(nullptr){}
        };
        Node* head_;
        Node* divider_;
        Node* tail_;
        std::atomic<int>size_;
public:
        LockFreeQueue(){
            head_=divider_=tail_=new Node(T{}); 
            size_=0;
        }
        ~LockFreeQueue(){
            while(head_!=nullptr){
                Node* tmp=head_;
                head_=head_->next;
                delete tmp;
            }
            size_=0;
        }
  
        void produce(const T& t){
            tail_->next=new Node(t);
            tail_=tail_->next;
            ++size_;

            while (head_!=divider_)
            {
                Node* tmp=head_;
                head_=head_->next;
                delete tmp;
            }
        }


        bool consume(T* result){
            if(divider_!=tail_){
                *result=divider_->next->data;
                divider_=divider_->next;
                --size_;
                return true;
            }
            return false;
        }
        bool empty(){
            return size_==0;
        }
        int size() {
            return size_;
        }
    };


} // namespace xrtc
