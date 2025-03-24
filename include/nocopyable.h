#pragma once

namespace muduo {

// 不可拷贝基类，继承该类的子类将无法拷贝构造和赋值
class nocopyable {
protected:
    nocopyable() = default;
    ~nocopyable() = default;

public:
    nocopyable(const nocopyable&) = delete;
    nocopyable& operator=(const nocopyable&) = delete;
};

} // namespace muduo