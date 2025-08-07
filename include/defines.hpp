#define USE_OPTIMIZED_THREAHOULD

#ifdef USE_OPTIMIZED_THREAHOULD
    #include <thread>
    #define INITIAL_NUM_THREADS std::thread::hardware_concurrency()
#else
    #define INITIAL_NUM_THREADS 10
#endif

#define MAX_NUM_THREADS 16
#define MAX_NUM_TASKS 1024

