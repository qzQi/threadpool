#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <functional>
#include <atomic>
#include <vector>
#include <future>
using namespace std;

// Task 线程池所保存的任务类型
using Task = function<void()>;

class ThreadPool {
  public:
    explicit ThreadPool(int n = thread::hardware_concurrency())
        : threadNums_(n), running(false) {}

    void start() {
        running = true;
        for (int i = 0; i < threadNums_; i++) {
            // 创建线程
            pool_.push_back(
                make_unique<thread>(std::bind(&ThreadPool::runInThread, this)));
        }
    }
    // addTask的返回类型应该是future<Type>其中type是func的返回类型
    // 但是这里我们也不知道是啥只能，使用尾置或者decltype(auto)
    template<typename Func, typename... Args>
    decltype(auto) addTask(Func &&func, Args &&...args);

    void stop() {
        running = false;
        // 唤醒所有阻塞的线程
        not_empty.notify_all();
        for (int i = 0; i<threadNums_; i++) {
            cout << pool_[i]->get_id() << " is joining" << endl;
            pool_[i]->join();
        }
    }
    ~ThreadPool() {
        if (running) { stop(); }
    }

  private:
    int threadNums_;
    atomic_bool running;
    deque<Task> deq;
    mutex mtx_;
    condition_variable not_empty; // 队列可以不限制任务数量
    vector<unique_ptr<thread>> pool_;
    // 线程的执行函数，通过bind传递给每个进程,
    // 消费者
    void runInThread() {
        while (running) {
            Task t;
            unique_lock<mutex> lk(mtx_);
            // 不能仅仅判断
            // empty，因为while会防止意外唤醒，这样就无法进行释放线程操作
            while (deq.empty() && running) { not_empty.wait(lk); }
            if (!deq.empty()) {
                t = deq.front();
                deq.pop_front();
            }
            if (t) { t(); }
        }
    }
};

// 就改了这几行？
template<typename Func,typename...Args>
decltype(auto) ThreadPool::addTask(Func&& func,Args&&...args){
    using RType=decltype(func(forward<Args>(args)...));

    // 没办法把packaged task直接使用lambda捕获，所以采用了折中的方案
    // 直接使用指针，虽然可能存在性能问题？ 但是实在是简单捏！
    auto ptask=make_shared<packaged_task<RType()>> (
        bind(forward<Func>(func),forward<Args>(args)...)
    );

    future<RType> res=ptask->get_future();
    unique_lock<mutex> lk(mtx_);

    // deq里的Task是function<void()> 使用lambda包装一下package
    deq.push_back([=](){
        (*ptask)();
    });
    not_empty.notify_one();
    return res;
}