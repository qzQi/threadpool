#include <functional>
#include <future>
#include <thread>
#include <iostream>
#include <chrono>
#include <memory>

using namespace std;

int func1(int a, int b) {
    // this_thread::sleep_for(chrono::seconds(3));
    return a + b;
}

// 测试task对象与function对象能不能互相转化？
// 答案是显而易见的packaged_task与function根本不是一种东西
// void task_funcion() {
//     packaged_task<int(int, int)> task1(func1);
//     future<int> res1 = task1.get_future();

//     // lambda能不能支持移动捕获？应该可以，这个场景常见
//     // 为什么不能用？
//     function<void(int, int)> f1([task1 = std::move(task1)](int a, int b) {
//         // task/packageed_task是没有返回值的
//         task1(a, b);
//     });

//     f1(1, 2);

//     cout << res1.get() << endl;
// }

// 测试将task传给线程，并使用future获取返回值
void task_thread() {
    packaged_task<int(int, int)> task1(func1);

    future<int> result1 = task1.get_future();
    thread th1(std::move(task1), 1, 2);
    cout << result1.get() << endl;
    th1.join();
}

// 测试直接创建task并执行
// 执行这个怎么会抛出异常？==> future&&packaged 的使用需要链接pthread，
// 因为底层使用了同步方法，get会一直阻塞等待到获得结果
void task_future() {
    packaged_task<int(int, int)> task1(func1);
    future<int> f1 = task1.get_future();

    task1(1, 2);
    cout << f1.get() << endl;
}

// 将packaged_task对象放入function执行！
// 肯定不能直接使用function包装，包装一下
void taskInFunction() {
    packaged_task<int(int,int)> task1(func1);
    future<int> res1=task1.get_future();

    // 昨天晚上都不通过
    auto func=[task1=std::move(task1)](int a,int b)mutable{
        task1(a,b);
    };
    func(1,2);
    cout<<res1.get()<<endl;
}

// 可以bind吗？==>可以bind适配成其他函数
// 可以bind，但是bind后的可读性，感觉不如使用lambda！
void task_bind(){
    packaged_task<int(int,int)> task1(func1);
    future<int> res1=task1.get_future();
    auto func=bind(std::move(task1),2,2);//auto 是什么？
    // function<void()> func{std::bind(std::move(task1),2,3)};
    func();
    cout<<res1.get()<<endl;
}

int main() {
    // task_funcion();
    // task_thread();
    // task_future();
    // taskInFunction();
    task_bind();
}