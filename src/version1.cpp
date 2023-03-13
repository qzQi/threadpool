#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <functional>
#include <atomic>
#include <vector>
using namespace std;

// bug:出现segment fault！
// for (int i = 0; threadNums_; i++) sb了

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
    // 生产者
    void addTask(Task t) {
        if (running) {
            unique_lock<mutex> lk(mtx_);
            deq.push_back(t);
            not_empty.notify_one();
        }
    }
    void stop() {
        // 调用stop进行回收线程为什么会segment fault
        running = false;
        // 唤醒所有阻塞的线程
        not_empty.notify_all();
        // 最后回收时候所有线程阻塞在not_empty，使用noify_all()唤醒后会，加锁
        // 应该不会线程安全问题，定位代码哪里出错！
        // 通过gdb调试发现是，上面notify的时候线程执行完runInthread了
        // 者nm也算是bug？ 无语了
        // for (int i = 0; threadNums_; i++) {
        //     // cout << pool_[i]->get_id() << " is joining" << endl;
        //     // pool_[i]->join();
        // }
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

uint64_t global = 0;
void func() {
    for (int i = 0; i < 1000000; i++) { global++; }
}

int main() {
    // ThreadPool pool();//直接照成歧义
    //
    ThreadPool pool{2};
    pool.start();
    // 出现段错误？==》非法内存访问，上面没有判断是否为空就获取que元素？
    // 还是segment fault
    for (int i = 0; i < 10; i++) { pool.addTask(func); }
    // 线程池能够正常工作，但是析构的时候出现段错误！内存相关的
    this_thread::sleep_for(chrono::seconds(3));
    cout << "now global " << global << endl;
    pool.stop();
    cout << global << endl;
}