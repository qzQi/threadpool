#include<bits/stdc++.h>

using namespace std;

// lambda确实可以通过移动捕获/初始化捕获
void fun1(){
    unique_ptr<int> uptrInt=make_unique<int>(2);
    auto func=[pint=std::move(uptrInt)](){
        cout<<*(pint)<<endl;
    };
    func();
}

int main(){
    fun1();
}