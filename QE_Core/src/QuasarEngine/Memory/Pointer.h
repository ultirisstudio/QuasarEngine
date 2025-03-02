#pragma once

#include <memory>
#include <iostream>

template <typename T, typename PtrType>
class Pointer
{
public:
    virtual ~Pointer() = default;

    virtual PtrType Create(T* ptr) = 0;
    virtual T* operator->() = 0;
    virtual T& operator*() = 0;
    virtual bool IsValid() const = 0;
    virtual PtrType Get() = 0;
};

template <typename T>
class UniquePointer : public Pointer<T, std::unique_ptr<T>>
{
public:
    explicit UniquePointer(T* ptr = nullptr) : m_ptr(ptr) {}

    std::unique_ptr<T> Create(T* ptr) override
    {
        return std::make_unique<T>(*ptr);
    }

    T* operator->() override { return m_ptr.get(); }
    T& operator*() override { return *m_ptr; }

    bool IsValid() const override { return m_ptr != nullptr; }

    std::unique_ptr<T>& Get() override { return m_ptr; }

private:
    std::unique_ptr<T> m_ptr;
};

template <typename T>
class SharedPointer : public Pointer<T, std::shared_ptr<T>>
{
public:
    explicit SharedPointer(std::shared_ptr<T> ptr = nullptr) : m_ptr(ptr) {}

    std::shared_ptr<T> Create(T* ptr) override
    {
        return std::make_shared<T>(*ptr);
    }

    T* operator->() override { return m_ptr.get(); }
    T& operator*() override { return *m_ptr; }

    bool IsValid() const override { return m_ptr != nullptr; }

    std::shared_ptr<T>& Get() override { return m_ptr; }

private:
    std::shared_ptr<T> m_ptr;
};