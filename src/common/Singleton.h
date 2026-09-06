#pragma once

// ============================================================================
// 单例模板 (Singleton 设计模式)
//
// 用法：
//   class Foo : public Singleton<Foo> {
//       friend class Singleton<Foo>;   // 让基类能访问私有构造
//   public:
//       void doSomething();
//   private:
//       Foo() = default;               // 私有构造, 禁止外部 new
//   };
//
//   Foo::instance().doSomething();     // 全局唯一实例
//
// 原理：C++11 起，函数内的局部静态变量在首次执行到该行时才构造，
//       且构造过程由编译器保证线程安全，因此无需手动加锁。
// ============================================================================

template <typename T>
class Singleton
{
public:
    // 获取唯一实例（线程安全，首次调用时构造）
    static T& instance()
    {
        static T s_instance;   // 局部静态变量：C++11 起初始化线程安全
        return s_instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

    // 禁止拷贝与赋值，保证实例唯一性
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
