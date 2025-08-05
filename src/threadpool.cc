#include "threadpool.hpp"

ThreadPool::ThreadPool():m_initialNumThreads(INITIAL_NUM_THREADS)
                        , m_taskQueueSize(0)
                        , m_maxTaskQueueSize(MAX_NUM_TASKS)
                        , m_maxNumThreads(MAX_NUM_THREADS)
                        , m_mode(ThreadPoolMode::MODE_FIXED){}

ThreadPool::~ThreadPool(){}

void ThreadPool::setMode(ThreadPoolMode mode){
    m_mode = mode;
}

void ThreadPool::setTaskQueueThreahold(size_t threahold){
    m_maxTaskQueueSize = threahold;
}

void ThreadPool::setInitThreadSize(size_t threahold){
    m_initialNumThreads = threahold;
}
void ThreadPool::start(){
    for(int i=0; i<m_initialNumThreads; i++){
        m_threads.emplace_back(new Thread([this]{
            threadHandler();
        }));
    }
    for(const auto& threadItem : m_threads){
        threadItem->start();
    }
}

void ThreadPool::threadHandler(){

}

//////////////////////////////////////

Thread::Thread(std::function<void()> func):m_func(func){}

int main(){}