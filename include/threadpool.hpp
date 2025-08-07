#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <unistd.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <iostream>
#include <format>
#include <future>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#define TASK_EXPIRE_SECONDS 1

class Thread{
public:
    using ThreadFuncType = std::function<void(std::stop_token, size_t)>;
    Thread(ThreadFuncType func);
    ~Thread();
    void start();
    void stop();
    size_t getId();
private:
    static size_t id_generator;
    ThreadFuncType m_func;
    std::unique_ptr<std::jthread> m_threadPtr;
    size_t m_id;
};

class TaskBase{
public:
    virtual void run() = 0;
    virtual ~TaskBase() = default;
};

template<typename ResType>
class Task : public TaskBase{
public:

    template<typename FuncType, typename ...Args>
    Task(FuncType func, Args... args){
        static_assert(!std::is_invocable_v<std::decay_t<FuncType>>, "not callable");
        m_task = std::make_shared<std::packaged_task<ResType()>>(
            std::bind(std::forward<FuncType>(func), std::forward<Args>(args)...)
        );
    }

    void run() override {
        (*m_task)();
    }
    
    std::future<ResType> getFuture(){
        return m_task->get_future();
    }

private:
    std::shared_ptr<std::packaged_task<ResType()>> m_task;
    // 这里可以再加一个优先级
};

class InvalidResult : std::exception{
public:
    InvalidResult(const char* msg):m_msg(msg){}
    const char* what() const noexcept override{
        return m_msg;
    }
private:
    const char* m_msg;
};

enum class ResultErrorInfo{
    ThreadPool_TaskQueueFull,
    ThreadPool_TimeOut,
    ThreadPool_Closed,
    ThreadPool_InvalidFuture,
    ThreadPool_Valid
};

template<typename ResType>
class Result{
public:

    Result(Result&&) noexcept = default;

    Result(std::future<ResType>&& t):m_f(std::move(t))
                                    ,isValid(true)
                                    ,info(ResultErrorInfo::ThreadPool_Valid){};

    ResType getValue() {
        if (!isValid){
            #define getErrorStr(x) #x
                throw InvalidResult(getErrorStr(info));
            #undef getErrorStr
        }
        return m_f.get();
    }

public:
    bool isValid;
    ResultErrorInfo info;
private:
    std::future<ResType> m_f;
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

    size_t createThread();

    void setTaskQueueThreahold(size_t threahold);

    void setInitThreadSize(size_t threahold);

    void setNumThreadThreahold(size_t threahold);

    void setTaskExpireSeconds(size_t seconds);

    void setThreadExpireSeconds(size_t seconds);

    void start();

    bool isPoolRunning();

    template<typename T>
    Result<T> submit(Task<T>&& task){
        std::cout<< "task submited\n";
        Result<T> res(std::move(task.getFuture()));
        std::shared_ptr<TaskBase> sp = std::make_shared<Task<T>>(std::move(task));

        // 线程池关闭时不允许提交任务
        if (!m_isRunning){
            std::cout<< "pool closed\n";
            res.isValid = false;
            res.info = ResultErrorInfo::ThreadPool_Closed;
            return res;
        }

        if (m_taskQueueSize == m_maxTaskQueueSize){
            std::cout<< "task full\n";
            res.isValid = false;
            res.info = ResultErrorInfo::ThreadPool_TaskQueueFull;
            return res;
        }
        // 获得锁
        std::unique_lock<std::mutex> l(m_mtx);
        // 等待任务队列为空
        bool isTimeOut = m_notFull.wait_for(l, std::chrono::seconds(TASK_EXPIRE_SECONDS), [this]()->bool{
            return m_taskQueueSize < m_maxTaskQueueSize;
        });
        // 超时处理
        if(!isTimeOut){
            std::cout<< "submit timeout\n";
            res.isValid = false;
            res.info = ResultErrorInfo::ThreadPool_TimeOut;
            return res;
        }

        m_taskQueue.emplace(sp);
        m_taskQueueSize++;
        m_notEmpty.notify_all();

        std::cout<< m_numIdleThread << m_taskQueueSize << m_numThreads << m_maxNumThreads <<std::endl;

        // 进行cached模式逻辑
        if(m_mode == ThreadPoolMode::MODE_CACHED 
            && m_numIdleThread < m_taskQueueSize 
            && m_numThreads < m_maxNumThreads){
            std::cout<< "add more threads\n";
            size_t id = createThread();
            m_threads[id]->start();
        }

        // 返回值
        return res;

    }

    template<typename FuncType, typename... Args>
    requires std::is_invocable_v<FuncType, Args...>
    auto submit(FuncType func, Args... args) 
        -> Result<std::invoke_result_t<std::decay_t<FuncType>, std::decay_t<Args>...>>{
        
        using ResType = std::invoke_result_t<std::decay_t<FuncType>, std::decay_t<Args>...>;
        return submit(Task<ResType>(std::forward<FuncType>(func), std::forward<Args>(args)...));
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void threadHandler(const std::stop_token& token, size_t thisThreadId);
private:
    ThreadPool();

    size_t m_taskExpireSeconds;
    size_t m_threadExpireSeconds;

    //std::vector<std::unique_ptr<Thread>> m_threads;         // 线程队列
    std::unordered_map<size_t, std::unique_ptr<Thread>> m_threads;
    size_t m_initialNumThreads;                             // 初始线程个数
    size_t m_maxNumThreads;                                 // 最大线程个数
    std::atomic_uint64_t m_numThreads;                       // 总线程数
    std::atomic_uint64_t m_numIdleThread;                   // 空闲线程数量


    std::queue<std::shared_ptr<TaskBase>> m_taskQueue;          // 任务队列
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