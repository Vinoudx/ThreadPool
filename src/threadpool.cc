#include "threadpool.hpp"

std::shared_ptr<ThreadPool> ThreadPool::m_ptr = nullptr;
std::once_flag ThreadPool::initFlag;

ThreadPool::ThreadPool():m_initialNumThreads(INITIAL_NUM_THREADS)
                        , m_taskQueueSize(0)
                        , m_maxTaskQueueSize(MAX_NUM_TASKS)
                        , m_maxNumThreads(MAX_NUM_THREADS)
                        , m_mode(ThreadPoolMode::MODE_FIXED)
                        , m_isRunning(true){}

ThreadPool::~ThreadPool(){
    while(m_taskQueueSize != 0){}
    m_isRunning = false;
    m_notEmpty.notify_all();
}

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
    m_isRunning = true;
    for(int i=0; i<m_initialNumThreads; i++){
        m_threads.emplace_back(std::make_unique<Thread>([this]{
            threadHandler();
        }));
    }
    for(const auto& threadItem : m_threads){
        threadItem->start();
    }
}

Result ThreadPool::submit(std::shared_ptr<Task> sp){
    // 线程池关闭时不允许提交任务
    if (!m_isRunning){
        //NotImplError
    }
    // 获得锁
    std::unique_lock<std::mutex> l(m_mtx);
    // 等待任务队列为空
    bool isTimeOut = m_notFull.wait_for(l, std::chrono::seconds(1), [this]()->bool{
        return m_taskQueueSize < m_maxTaskQueueSize;
    });
    // 超时处理
    if(!isTimeOut){
        //NotImplError
    }

    m_taskQueue.emplace(sp);
    m_taskQueueSize++;
    m_notEmpty.notify_all();

}

void ThreadPool::threadHandler(){
    std::cout<< "thread begin "<< std::this_thread::get_id() << '\n';
    while(true){
        std::unique_lock<std::mutex> l(m_mtx);
        m_notEmpty.wait(l, [this]()->bool{
            return !m_isRunning || !m_taskQueue.empty();
        });

        if(!m_isRunning)break;

        auto task = m_taskQueue.front();
        m_taskQueue.pop();
        m_taskQueueSize--;
        l.unlock();

        task->run();
        m_notFull.notify_all();
    }
    std::cout<< "thread end "<< std::this_thread::get_id() << '\n';
}

//////////////////////////////////////

Thread::Thread(ThreadFuncType func):m_func(func){}

Thread::~Thread(){
    m_threadPtr->join();
}

void Thread::start(){
    m_threadPtr = make_unique<std::thread>(m_func);
}

int main(){
    auto threadpool = ThreadPool::getThreadPool();
    threadpool->setInitThreadSize(4);
    threadpool->start();
}