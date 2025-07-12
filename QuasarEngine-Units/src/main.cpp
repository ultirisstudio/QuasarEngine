#include <iostream>
#include <chrono>
#include <vector>
#include <cassert>
#include <memory>

#include <QuasarEngine/Memory/Pointer.h>

namespace QuasarEngine
{
    struct MyObject
    {
        int a;
        double b;

        MyObject() : a(0), b(0.0)
        {
            std::cout << "MyObject() ctor\n";
        }

        MyObject(int x, double y) : a(x), b(y)
        {
            std::cout << "MyObject(int, double) ctor\n";
        }

        ~MyObject()
        {
            std::cout << "~MyObject() dtor\n";
        }

        void hello() const
        {
            std::cout << "Hello MyObject: a=" << a << ", b=" << b << "\n";
        }
    };

    void TestUniquePointer()
    {
        std::cout << "\n-- Test UniquePointer --\n";

        UniquePointer<MyObject> uptr = MakeUnique<MyObject>(42, 3.14);

        assert(uptr.IsValid());
        uptr->hello();
        assert((*uptr).a == 42);

        UniquePointer<MyObject> uptr2 = std::move(uptr);
        assert(!uptr.IsValid());
        assert(uptr2.IsValid());

        uptr2.Reset();
        assert(!uptr2.IsValid());

        std::cout << "UniquePointer tests OK\n";
    }

    void TestSharedPointer()
    {
        std::cout << "\n-- Test SharedPointer --\n";

        SharedPointer<MyObject> sptr = MakeShared<MyObject>(7, 2.71);

        assert(sptr.IsValid());
        sptr->hello();

        {
            SharedPointer<MyObject> sptr2 = sptr;
            assert(sptr2.IsValid());
            assert(sptr2->a == 7);
            sptr2->hello();
        }

        assert(sptr.IsValid());

        sptr.Reset();
        assert(!sptr.IsValid());

        std::cout << "SharedPointer tests OK\n";
    }

    void TestWeakPointer()
    {
        std::cout << "\n-- Test WeakPointer --\n";

        SharedPointer<MyObject> sptr = MakeShared<MyObject>(99, 1.618);

        WeakPointer<MyObject> wptr(sptr.Get());
        assert(wptr.IsValid());
        wptr->hello();

        sptr.Reset();
        assert(!wptr.IsValid());

        std::cout << "WeakPointer tests OK\n";
    }

    void TestMemoryTrackerBalance()
    {
        std::cout << "\n-- Test MemoryTracker balance --\n";

        auto& tracker = MemoryTracker::instance();

        size_t before = tracker.GetCurrentUsage();

        {
            UniquePointer<MyObject> uptr = MakeUnique<MyObject>();
            assert(tracker.GetCurrentUsage() >= before);

            SharedPointer<MyObject> sptr = MakeShared<MyObject>();
            assert(tracker.GetCurrentUsage() >= before);
        }

        assert(tracker.GetCurrentUsage() == before);

        std::cout << "MemoryTracker balance test OK\n";
    }

    void BenchmarkPointerCreation(int count = 100000)
    {
        std::cout << "\n-- Benchmark Pointer Creation x" << count << " --\n";

        using Clock = std::chrono::high_resolution_clock;

        {
            UniquePointer<MyObject> uptr;
            auto start = Clock::now();
            for (int i = 0; i < count; ++i)
            {
                uptr = MakeUnique<MyObject>(i, double(i) * 0.1);
            }
            auto end = Clock::now();
            std::cout << "UniquePointer creation time: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                << " ms\n";
        }

        {
            SharedPointer<MyObject> sptr;
            auto start = Clock::now();
            for (int i = 0; i < count; ++i)
            {
                sptr = MakeShared<MyObject>(i, double(i) * 0.1);
            }
            auto end = Clock::now();
            std::cout << "SharedPointer creation time: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                << " ms\n";
        }
    }
}

int main()
{
    QuasarEngine::TestUniquePointer();
    QuasarEngine::TestSharedPointer();
    QuasarEngine::TestWeakPointer();
    QuasarEngine::TestMemoryTrackerBalance();
    QuasarEngine::BenchmarkPointerCreation();

    std::cout << "\nAll tests passed!\n";
	return 0;
}