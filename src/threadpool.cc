#include "threadpool.hpp"
#define INITIAL_NUM_THREADS std::thread::hardware_concurrency()
#define MAX_NUM_THREADS 64
#define MAX_NUM_TASKS 1024
#define THREAD_EXPIRE_SECONDS 5
#define TASK_EXPIRE_SECONDS 1

std::shared_ptr<ThreadPool> ThreadPool::m_ptr = nullptr;
std::once_flag ThreadPool::initFlag;
size_t Thread::id_generator = 0;

ThreadPool::ThreadPool():m_initialNumThreads(INITIAL_NUM_THREADS)
                        , m_taskQueueSize(0)
                        , m_maxTaskQueueSize(MAX_NUM_TASKS)
                        , m_maxNumThreads(MAX_NUM_THREADS)
                        , m_mode(ThreadPoolMode::MODE_FIXED)
                        , m_isRunning(false)
                        , m_numIdleThread(0)
                        , m_numThreads(0)
                        , m_taskExpireSeconds(TASK_EXPIRE_SECONDS)
                        , m_threadExpireSeconds(THREAD_EXPIRE_SECONDS){}

ThreadPool::~ThreadPool(){
    m_isRunning = false;
    while(m_taskQueueSize != 0){}
    for(const auto& item: m_threads){
        item.second->stop();
    }
    m_notEmpty.notify_all();    
}

bool ThreadPool::isPoolRunning(){
    return m_isRunning;
}

void ThreadPool::setMode(ThreadPoolMode mode){
    if(isPoolRunning()){
        return;
    }
    m_mode = mode;
}

void ThreadPool::setTaskQueueThreahold(size_t threahold){
    if(isPoolRunning()){
        return;
    }
    m_maxTaskQueueSize = threahold;
}

void ThreadPool::setNumThreadThreahold(size_t threahold){
    if(isPoolRunning()){
        return;
    }
    m_maxNumThreads = threahold;
}

void ThreadPool::setTaskExpireSeconds(size_t seconds){
    if(isPoolRunning()){
        return;
    }
    m_taskExpireSeconds = seconds;
}

void ThreadPool::setThreadExpireSeconds(size_t seconds){
    if(isPoolRunning()){
        return;
    }
    m_threadExpireSeconds = seconds;
}

void ThreadPool::setInitThreadSize(size_t threahold){
    if(isPoolRunning()){
        return;
    }
    m_initialNumThreads = threahold;
}
void ThreadPool::start(){
    m_isRunning = true;
    for(int i=0; i<m_initialNumThreads; i++){
        createThread();
    }
    for(const auto& threadItem : m_threads){
        threadItem.second->start();
    }
}

size_t ThreadPool::createThread(){

    auto ptr = std::make_unique<Thread>([this](std::stop_token token, size_t thisThreadId){
        threadHandler(token, thisThreadId);
    });
    size_t id = ptr->getId();
    m_threads.emplace(id, std::move(ptr));
    m_numIdleThread++;
    m_numThreads++;
    return id;
}


void ThreadPool::threadHandler(const std::stop_token& token, size_t thisThreadId){
    std::cout<< "thread begin "<< thisThreadId<< '\n';
    while(true){
        std::unique_lock<std::mutex> l(m_mtx);

        if(m_mode == ThreadPoolMode::MODE_CACHED){
            bool isexpired = m_notEmpty.wait_for(l,std::chrono::seconds(THREAD_EXPIRE_SECONDS) , [this, &token]()->bool{
                return token.stop_requested() || !m_taskQueue.empty();
            });
            
            // 线程超时
            if(!isexpired){
                if(m_numThreads > m_initialNumThreads){
                    std::cout<< "thread expired "<< thisThreadId << '\n';
                    // 回收线程
                    m_numThreads--;
                    m_numIdleThread--;
                    // 把线程对象从容器中删除
                    m_threads.erase(thisThreadId);
                    break;
                }else{
                    continue;
                }
            }

        }else{
            m_notEmpty.wait(l, [this, &token]()->bool{
                return token.stop_requested() || !m_taskQueue.empty();
            });
        }

        if(token.stop_requested())break;
        
        m_numIdleThread--;

        if(!m_taskQueue.empty()){
            m_notEmpty.notify_all();
        }
        std::cout<< "get task "<< thisThreadId << '\n';
        auto task = m_taskQueue.front();
        m_taskQueue.pop();
        m_taskQueueSize--;
        l.unlock();

        task->run();
        m_notFull.notify_all();

        m_numIdleThread++;
    }
    std::cout<< "thread end "<< thisThreadId << '\n';
}

//////////////////////////////////////

Thread::Thread(ThreadFuncType func):m_func(func){
    m_id = id_generator;
    id_generator++;
}

Thread::~Thread(){
    if(m_threadPtr->joinable()){
        m_threadPtr->detach();
    }
}

void Thread::start(){
    m_threadPtr = make_unique<std::jthread>(m_func, m_id);
}

void Thread::stop(){
    m_threadPtr->request_stop();
}

size_t Thread::getId(){
    return m_id;
}

