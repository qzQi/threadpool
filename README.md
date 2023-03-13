# threadpool

由muduo里面base库的线程池改进而来。  
目标：
* 提供更加方便的使用形式        
  这个倒是很小的改进，直接使用variadic template即可
  ```C++
  //muduo版本的线程池使用：src/version1
  using Task=function<void()>;
  void addTask(Task t);

  //当我们想加入一个Task
  void test1(int,int,int);
  pool.addTask(std::bind(test1,1,2,3));

  //改进目标：直接放入函数&参数
  pool.addTask(test1,1,2,3);
  ```
* 提供获取线程执行函数的返回值      
   如何获取线程执行后的返回值？标准库为我们提供了工具，那就是packaged_task&&future。
   为了获取返回值，我们使用packaged_task包装一下需要执行的函数，提交任务时候返回给用户一个future。
   ```C++
   //设计目标：
   int func1(int,int,int );
   future<int> res1=addTask(func1,1,2,3);

   //需要线程结果时候
   cout<<res1.get()<<endl;
   ```
   为了实现这个目标有两个需要解决：    
   * `addTask`函数的返回类型，怎么判断？
   * 任务队列我的定义是`deque<Task>`，如何将packaged_task放入function？

## packaged_task讲解
类模板 std::packaged_task 包装任何可调用 (Callable) 目标（函数、 lambda 表达式、 bind 表达式或其他函数对象），使得能异步调用它。其返回值或所抛异常被存储于能通过 std::future 对象访问的共享状态中。
和function的使用类似，都是可以包装可调用对象，然后也可作为thread类的执行函数（注意需要move）。       

注意packaged_task需要被调用future才能返回；并且packaged_task是没有返回值的，它的返回值需要通过future得到。

`test/packaged`下面写了一些packaged_task的测试：
* 作为线程thread的调用对象
* 与function的结合
* 与lambda的结合
* 与bind的结合
* 配合future的使用

C++的线程类并没有提供获取线程返回值方法，它的使用形式如下：
```C++
void func(int a,int b){
    //do sth
}
thread th(func,1,2);
th.join();
```
thread接受的是一个可调用对象和它的参数，可调用对象可以是packaged_task或者function。


## 详解
目前来看仅仅需要修改addTask，返回一个future，接收任意个数/类型参数。
```C++
    // 生产者
    void addTask(Task t) {
        if (running) {
            unique_lock<mutex> lk(mtx_);
            deq.push_back(t);
            not_empty.notify_one();
        }
    }
```

改动后的代码：      
相关内容：尾置返回类型，variadic template
```C++
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
```

## end
`src/version1`是参考muduo的线程池实现。       
`include/ThreadPool.h`是改进后的实现。

改进后的使用方法：
```C++
int func(int a,int b,int c);
ThreadPool pool{};

pool.start();

future<int> res=pool.addTask(func,1,2,3);

cout<<res.get()<<endl;
```