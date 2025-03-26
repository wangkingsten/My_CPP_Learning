/*
手撕 shared_ptr

实现接口：
1、构造函数：不带参数、带参数的（传指针）
2、析构函数
3、拷贝构造函数
4、拷贝赋值运算符
5、移动构造函数
6、移动赋值运算符
7、解引用操作
8、箭头运算符
9、引用计数、原始指针、重置指针

要求：
1、不用考虑删除器和空间配置器
2、不用考虑弱引用
3、考虑引用计数器的线程安全
4、提出测试案例

细节：
互斥锁、原子操作、内存序、指令重排


*/

#pragma once

#include <atomic> // 原子操作
#include <memory>

template <typename T>
class shared_ptr {

private:
    T* _ptr; //指向被管理的对象
    std::atomic<std::size_t> * _ref_count;  // 引用计数

private:
    // 引用计数-1，如果引用计数为0，则回收堆空间
    void release() {
        if(_ref_count && _ref_count->fetch_sub(1,std::memory_order_acq_rel) == 1) {
            std::default_delete<T>{}(_ptr); 
            // 使用 std::default_delete 处理自定义类型
            // delete _ptr; 直接调用 delete，无法处理数组类型 T[]。
            // 使用 std::default_delete<T>{}(_ptr);，这样 shared_ptr<int[]> 之类的情况也可以正确处理。
            delete _ref_count;
        }

    }

public:
    // 默认构造函数
    shared_ptr()
    :_ptr(nullptr),_ref_count(nullptr)
    {}

    // 带参数的构造函数
    // 这里使用explicit关键字，防止隐式类型转换  
    // shared_ptr<int> sp = new int(200);会触发隐式类型转换，不希望这样
    // shared_ptr<int> sp = shared_ptr<int> (new int(200)); 希望这样
    explicit shared_ptr(T* p)
    :_ptr(p) {
        if(p) {
            _ref_count = new std::atomic<std::size_t>(1);
        } else {
            _ref_count = nullptr;
        }
    }

    // 析构函数
    ~shared_ptr() {
        release();
    }

    // 拷贝构造函数
    // 注意细节参数为常引用
    shared_ptr(const shared_ptr<T> & other)
    :_ptr(other._ptr),_ref_count(other._ref_count) {
        if(_ref_count) {
            _ref_count->fetch_add(1, std::memory_order_relaxed); 
            // 引用计数增加，使用 fetch_add原子操作实现
            // 内存序不熟悉的话，可以不用填第二个参数
            // 这里使用松散的内存序，因为增加的话不会为0，先加后加没有太大影响
        }
    }

    // 拷贝赋值运算符
    // 细节：需要判断自赋值的情况
    shared_ptr<T> & operator=(const shared_ptr<T> & other) {
        if(this != &other) {
            // 释放当前资源
            release();
            _ptr = other._ptr;
            _ref_count = other._ref_count;
            if (_ref_count) {
                _ref_count->fetch_add(1, std::memory_order_relaxed);
                // 引用计数增加，使用 fetch_add原子操作实现
                // 内存序不熟悉的话，可以不用填第二个参数
                // 这里使用松散的内存序，因为增加的话不会为0，先加后加没有太大影响
            }
        }
        return *this;
    }

    // 移动构造函数
    // 细节 noexcept 告诉编译器不会抛出异常，在确定不会抛出异常的情况下再使用
    // 好处：如果用vector存放shared_ptr的话，其中的移动操作要求是noexcept std::vector<shared_ptr<T>> tem;
    // 也可以帮助编译器生成高效的代码
    shared_ptr(shared_ptr<T> && other) noexcept
    :_ptr(other._ptr), _ref_count(other._ref_count) {
        other._ptr = nullptr;
        other._ref_count = nullptr;
    }

    // 移动赋值运算符
    shared_ptr<T> & operator=(shared_ptr<T> && other) noexcept {
        if(this != &other) {
            // 判断自赋值
            release();// 先回收掉之前的资源，重点不要忘记
            _ptr = other._ptr;
            _ref_count = other._ref_count;
            other._ptr = nullptr;
            other._ref_count = nullptr;
        }
        return *this;
    }

    // 解引用运算符
    // 细节const ,表示该函数不会修改对象的状态
    // const在这里修饰的是this指针，this指针原本是方向确定，现在左定值，现在this指向的值也不能通过这个改动了
    T& operator*() const {
        return *_ptr;
    }

    // 箭头运算符
    T* operator->() const {
        return _ptr;
    }

    // 获取引用计数
    // 注意着个内存序，不了解不要填
    std::size_t use_count() const {
        return _ref_count ? _ref_count->load(std::memory_order_acquire) : 0;
    }

    // 获取裸指针
    T* get() const {
        return _ptr;
    }

    // 重置指针
    void reset(T* p = nullptr) {
        release();
        _ptr = p;
        _ref_count = p ? new std::atomic<std::size_t>(1) : nullptr;
    }
};