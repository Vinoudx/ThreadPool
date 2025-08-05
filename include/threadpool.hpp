#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <vector>
#include <unistd.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "defines.hpp"

class Thread{
public:
    Thread(std::function<void()> func);
    void start();
private:
    std::function<void()> m_func;
};

class Task{
public:
    virtual void run() = 0;
};

class Result{
    
};

enum class ThreadPoolMode{
    MODE_FIXED,
    MODE_CACHED
};

class ThreadPool{
public:

    ~ThreadPool();

    template<typename ...Args>
    static std::shared_ptr<ThreadPool> getThreadPool(Args&&... args){
        if(m_ptr == nullptr){
            m_ptr = std::make_shared<ThreadPool>(std::forward<Args>(args)...);
        }
        return m_ptr;
    }

    void setMode(ThreadPoolMode mode);

    void setTaskQueueThreahold(size_t threahold);

    void setInitThreadSize(size_t threahold);

    void start();

    Result submit(std::shared_ptr<Task> sp);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void threadHandler();
private:
    ThreadPool();

    std::vector<std::shared_ptr<Thread>> m_threads;         // 线程队列
    size_t m_initialNumThreads;                             // 初始线程个数
    size_t m_maxNumThreads;                                 // 最大线程个数

    std::queue<std::shared_ptr<Task>> m_taskQueue;          // 任务队列
    size_t m_maxTaskQueueSize;                              // 最大任务数量
    std::atomic_uint64_t m_taskQueueSize;                   // 任务数量

    std::mutex mtx;                                         // 对任务队列的互斥锁
    std::condition_variable m_notFull;                      // 分别对生产者和消费者的条件变量
    std::condition_variable m_notEmpty;                     

    ThreadPoolMode m_mode;                                  // 线程池模式

    static std::shared_ptr<ThreadPool> m_ptr;                      // 单例指针
};

#endif