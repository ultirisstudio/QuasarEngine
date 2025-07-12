#pragma once

#include <memory>
#include <new>
#include <utility>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <sstream>
#include <stdexcept>

#include "Allocator.h"

namespace QuasarEngine
{
    inline size_t GenerateUniqueID()
    {
        static std::atomic_size_t counter{ 0 };
        return ++counter;
    }

    inline std::string CaptureStackTrace()
    {
        std::stringstream ss;
        ss << "StackTrace at thread " << std::this_thread::get_id();
        return ss.str();
    }

    struct AllocationInfo
    {
        size_t size;
        std::string stackTrace;
    };

    namespace MemoryTracking
    {
        inline std::unordered_map<void*, AllocationInfo> g_allocations;
        inline std::mutex g_allocMutex;

        inline void RegisterAllocation(void* addr, size_t size)
        {
            std::lock_guard<std::mutex> lock(g_allocMutex);
            g_allocations[addr] = { size, CaptureStackTrace() };
        }

        inline size_t UnregisterAllocation(void* addr)
        {
            std::lock_guard<std::mutex> lock(g_allocMutex);
            auto it = g_allocations.find(addr);
            if (it != g_allocations.end())
            {
                size_t size = it->second.size;
                g_allocations.erase(it);
                return size;
            }
            return 0;
        }
    }

    template <typename T>
    void DefaultDeleter(T* ptr)
    {
        if (ptr)
        {
            void* addr = static_cast<void*>(ptr);
            ptr->~T();
            size_t size = MemoryTracking::UnregisterAllocation(addr);
            alignedFree(addr, size);
        }
    }

    template <typename T, typename PtrType>
    class Pointer
    {
    public:
        virtual ~Pointer() = default;

        virtual T* operator->() = 0;
        virtual T& operator*() = 0;
        virtual bool IsValid() const = 0;
        virtual PtrType& Get() = 0;

        virtual void Reset() = 0;
        virtual void Swap(Pointer& other) = 0;
        virtual explicit operator bool() const = 0;
    };

    template <typename T>
    class UniquePointer : public Pointer<T, std::unique_ptr<T, void(*)(T*)>>
    {
    public:
        using DeleterType = void(*)(T*);

        UniquePointer()
            : m_ptr(nullptr, &DefaultDeleter<T>)
        {
        }

        explicit UniquePointer(std::unique_ptr<T, DeleterType>&& ptr)
            : m_ptr(std::move(ptr))
        {
        }

        T* operator->() override { return m_ptr.get(); }
        T& operator*() override { return *m_ptr; }
        bool IsValid() const override { return m_ptr != nullptr; }
        explicit operator bool() const override { return IsValid(); }
        std::unique_ptr<T, DeleterType>& Get() override { return m_ptr; }
        void Reset() override { m_ptr.reset(); }
        void Swap(Pointer<T, std::unique_ptr<T, DeleterType>>& other) override
        {
            auto& otherPtr = static_cast<UniquePointer<T>&>(other).m_ptr;
            m_ptr.swap(otherPtr);
        }

    private:
        std::unique_ptr<T, DeleterType> m_ptr;
    };

    template <typename T>
    class SharedPointer : public Pointer<T, std::shared_ptr<T>>
    {
    public:
        SharedPointer()
            : m_ptr(nullptr)
        {
        }

        explicit SharedPointer(std::shared_ptr<T> ptr)
            : m_ptr(std::move(ptr))
        {
        }

        T* operator->() override { return m_ptr.get(); }
        T& operator*() override { return *m_ptr; }
        bool IsValid() const override { return m_ptr != nullptr; }
        explicit operator bool() const override { return IsValid(); }
        std::shared_ptr<T>& Get() override { return m_ptr; }
        void Reset() override { m_ptr.reset(); }
        void Swap(Pointer<T, std::shared_ptr<T>>& other) override
        {
            auto& otherPtr = static_cast<SharedPointer<T>&>(other).m_ptr;
            m_ptr.swap(otherPtr);
        }

        std::weak_ptr<T> ToWeak() const { return m_ptr; }

    private:
        std::shared_ptr<T> m_ptr;
    };

    template <typename T>
    class WeakPointer : public Pointer<T, std::weak_ptr<T>>
    {
    public:
        WeakPointer() = default;

        explicit WeakPointer(const std::shared_ptr<T>& sharedPtr)
            : m_ptr(sharedPtr)
        {
        }

        T* operator->() override
        {
            auto sp = m_ptr.lock();
            if (!sp)
                throw std::runtime_error("Dereferencing expired weak pointer");
            return sp.get();
        }

        T& operator*() override
        {
            auto sp = m_ptr.lock();
            if (!sp)
                throw std::runtime_error("Dereferencing expired weak pointer");
            return *sp;
        }

        bool IsValid() const override
        {
            return !m_ptr.expired();
        }
        explicit operator bool() const override
        {
            return IsValid();
        }
        std::weak_ptr<T>& Get() override { return m_ptr; }
        void Reset() override { m_ptr.reset(); }
        void Swap(Pointer<T, std::weak_ptr<T>>& other) override
        {
            auto& otherPtr = static_cast<WeakPointer<T>&>(other).m_ptr;
            m_ptr.swap(otherPtr);
        }
        void Set(const std::shared_ptr<T>& sharedPtr)
        {
            m_ptr = sharedPtr;
        }

    private:
        std::weak_ptr<T> m_ptr;
    };

    template<typename T, typename... Args>
    UniquePointer<T> MakeUnique(Args&&... args)
    {
        size_t size = sizeof(T);
        void* mem = alignedAlloc(alignof(T), size);
        if (!mem) throw std::bad_alloc();

        T* obj = nullptr;
        try
        {
            obj = new(mem) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            alignedFree(mem, size);
            throw;
        }

        MemoryTracking::RegisterAllocation(mem, size);

        std::unique_ptr<T, void(*)(T*)> uptr(obj, &DefaultDeleter<T>);
        return UniquePointer<T>(std::move(uptr));
    }

    template<typename T, typename... Args>
    SharedPointer<T> MakeShared(Args&&... args)
    {
        size_t size = sizeof(T);
        void* mem = alignedAlloc(alignof(T), size);
        if (!mem) throw std::bad_alloc();

        T* obj = nullptr;
        try
        {
            obj = new(mem) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            alignedFree(mem, size);
            throw;
        }

        MemoryTracking::RegisterAllocation(mem, size);

        std::shared_ptr<T> sptr(obj, &DefaultDeleter<T>);
        return SharedPointer<T>(std::move(sptr));
    }
}
