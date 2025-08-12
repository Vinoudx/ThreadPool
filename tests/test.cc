#include "threadpool.hpp"
#include <iostream>

unsigned long long func(const int& j){
    unsigned long long res = 0;
    for(unsigned long long i=0; i<j;i++){
        res += i;
    }
    return res;
}

class A{
public:
    A(int b):a(b){}

    int operator()(int b){
        return a + b;
    }

private:
    int a;
};

int main(){
    auto threadPoolItem = ThreadPool::getThreadPool();
    threadPoolItem->setInitThreadSize(2);
    threadPoolItem->setMode(ThreadPoolMode::MODE_CACHED);
    threadPoolItem->setTaskExpireSeconds(5);
    threadPoolItem->start();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int i=10000000;

    Result<unsigned long long> res1 = threadPoolItem->submit(Task<unsigned long long>(func, i));
    Result<unsigned long long> res2 = threadPoolItem->submit(Task<unsigned long long>(func, i));
    Result<unsigned long long> res3 = threadPoolItem->submit(Task<unsigned long long>(func, i));
    Result<unsigned long long> res4 = threadPoolItem->submit(func, i);
    Result<unsigned long long> res5 = threadPoolItem->submit(func, i);
    Result<unsigned long long> res6 = threadPoolItem->submit(func, i);


    std::cout<< res1.getValue()
    << res2.getValue()
    << res3.getValue()
    << res4.getValue()
    << res5.getValue()
    << res6.getValue()
    << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(10));


    return 0;
}