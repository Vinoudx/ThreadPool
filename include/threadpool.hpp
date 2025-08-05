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
#include <iostream>
#include <format>

#include "defines.hpp"

class Thread{
public:
    using ThreadFuncType = std::function<void()>;
    Thread(ThreadFuncType func);
    ~Thread();
    void start();
private:
    ThreadFuncType m_func;
    std::unique_ptr<std::thread> m_threadPtr;
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
        std::call_once(initFlag, [&args...]{
            m_ptr.reset(new ThreadPool(std::forward<Args>(args)...));
        });
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

    std::vector<std::unique_ptr<Thread>> m_threads;         // 线程队列
    size_t m_initialNumThreads;                             // 初始线程个数
    size_t m_maxNumThreads;                                 // 最大线程个数

    std::queue<std::shared_ptr<Task>> m_taskQueue;          // 任务队列
    size_t m_maxTaskQueueSize;                              // 最大任务数量
    std::atomic_uint64_t m_taskQueueSize;                   // 任务数量

    std::mutex m_mtx;                                         // 对任务队列的互斥锁
    std::condition_variable m_notFull;                      // 分别对生产者和消费者的条件变量
    std::condition_variable m_notEmpty;                     
    std::atomic_bool m_isRunning;                             // 线程池是否正在运行

    ThreadPoolMode m_mode;                                  // 线程池模式

    static std::shared_ptr<ThreadPool> m_ptr;                      // 单例指针
    static std::once_flag initFlag;
};

#endif