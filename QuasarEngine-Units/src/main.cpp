#include <iostream>
#include <chrono>
#include <vector>
#include <cassert>
#include <memory>
#include <numeric>
#include <thread>

#include <QuasarEngine/Memory/Pointer.h>

#include <QuasarEngine/Thread/JobSystem.h>
#include <QuasarEngine/Thread/ThreadPool.h>

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

        auto& tracker = MemoryTracker::Instance();

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

    struct ScopeTimer
    {
        const char* name;
        std::chrono::high_resolution_clock::time_point start;

        explicit ScopeTimer(const char* n)
            : name(n)
            , start(std::chrono::high_resolution_clock::now())
        {
        }

        ~ScopeTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << name << " : " << ms << " ms\n";
        }
    };

    void TestSimpleJobs()
    {
        std::cout << "==== TestSimpleJobs ====\n";

        JobSystem jobSystem;

        std::atomic<int> counter{ 0 };
        const int N = 1000;

        {
            ScopeTimer timer("JobSystem 1000 incréments");

            std::vector<std::future<void>> futures;
            futures.reserve(N);

            for (int i = 0; i < N; ++i)
            {
                auto fut = jobSystem.Submit(
                    JobPriority::NORMAL,
                    JobPoolType::GENERAL,
                    [&counter]()
                    {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                );

                futures.push_back(std::move(fut));
            }

            for (auto& f : futures)
                f.get();
        }

        std::cout << "Counter = " << counter.load() << " (attendu : " << N << ")\n";
        assert(counter.load() == N);
        std::cout << "TestSimpleJobs OK\n\n";
    }

    void TestDependencies()
    {
        std::cout << "==== TestDependencies ====\n";

        JobSystem jobSystem;

        std::vector<int> values;
        values.reserve(3);

        auto futA = jobSystem.Submit(
            JobPriority::NORMAL,
            JobPoolType::GENERAL,
            [&values]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                values.push_back(1);
            }
        );

        auto futB = jobSystem.Submit(
            JobPriority::NORMAL,
            JobPoolType::GENERAL,
            [&values]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                values.push_back(2);
            }
        );

        auto jobA = std::make_shared<Job>();
        jobA->name = "JobA";
        jobA->priority = JobPriority::NORMAL;
        jobA->pool = JobPoolType::GENERAL;
        jobA->func = [&values]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            values.push_back(10);
            };

        auto jobB = std::make_shared<Job>();
        jobB->name = "JobB";
        jobB->priority = JobPriority::NORMAL;
        jobB->pool = JobPoolType::GENERAL;
        jobB->func = [&values]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            values.push_back(20);
            };

        auto futA2 = jobSystem.Submit(
            JobPriority::NORMAL,
            JobPoolType::GENERAL,
            std::vector<Job::Ptr>{},
            "JobA",
            [func = jobA->func]() { func(); }
        );

        auto futB2 = jobSystem.Submit(
            JobPriority::NORMAL,
            JobPoolType::GENERAL,
            std::vector<Job::Ptr>{},
            "JobB",
            [func = jobB->func]() { func(); }
        );

        std::vector<Job::Ptr> deps;
        deps.push_back(jobA);
        deps.push_back(jobB);

        auto futC = jobSystem.Submit(
            JobPriority::HIGH,
            JobPoolType::GENERAL,
            deps,
            "JobC",
            [&values]() -> int
            {
                int sum = 0;
                for (int v : values)
                    sum += v;
                return sum;
            }
        );

        futA.get();
        futB.get();
        futA2.get();
        futB2.get();
        int resultC = futC.get();

        std::cout << "values.size() = " << values.size() << "\n";
        std::cout << "Somme calculée dans C = " << resultC << "\n";

        assert(resultC >= 30);

        std::cout << "TestDependencies OK\n\n";
    }

    void TestStress()
    {
        std::cout << "==== TestStress ====\n";

        const std::size_t N = 10'000'000;
        std::vector<int> data(N);
        std::iota(data.begin(), data.end(), 1);

        long long seqSum = 0;
        {
            ScopeTimer timer("Somme séquentielle");
            seqSum = std::accumulate(data.begin(), data.end(), 0LL);
        }

        JobSystem jobSystem;

        long long parallelSum = 0;
        {
            ScopeTimer timer("Somme parallèle (JobSystem)");

            const std::size_t grain = 100'000;
            const std::size_t chunkCount = (N + grain - 1) / grain;

            std::vector<std::future<long long>> futures;
            futures.reserve(chunkCount);

            for (std::size_t c = 0; c < chunkCount; ++c)
            {
                std::size_t begin = c * grain;
                std::size_t end = std::min(N, begin + grain);

                if (begin >= end)
                    break;

                auto fut = jobSystem.Submit(
                    JobPriority::NORMAL,
                    JobPoolType::GENERAL,
                    [begin, end, &data]() -> long long
                    {
                        long long s = 0;
                        for (std::size_t i = begin; i < end; ++i)
                            s += data[i];
                        return s;
                    }
                );

                futures.push_back(std::move(fut));
            }

            for (auto& f : futures)
            {
                parallelSum += f.get();
            }
        }

        std::cout << "Somme séquentielle : " << seqSum << "\n";
        std::cout << "Somme parallèle    : " << parallelSum << "\n";

        assert(seqSum == parallelSum);
        std::cout << "TestStress OK\n\n";
    }

    void TestThreadPool()
    {
        std::cout << "==== TestThreadPool ====\n";

        ThreadPool pool(4);

        std::atomic<int> counter{ 0 };
        const int N = 1000;

        std::vector<std::future<void>> futures;
        futures.reserve(N);

        {
            ScopeTimer timer("ThreadPool 1000 incréments");

            for (int i = 0; i < N; ++i)
            {
                auto fut = pool.Enqueue(
                    [&counter]()
                    {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                );
                futures.push_back(std::move(fut));
            }
        }

        for (auto& f : futures)
            f.get();

        std::cout << "Counter = " << counter.load() << " (attendu : " << N << ")\n";
        assert(counter.load() == N);

        pool.Shutdown();

        std::cout << "TestThreadPool OK\n\n";
    }
}

int main()
{
    //QuasarEngine::TestUniquePointer();
    //QuasarEngine::TestSharedPointer();
    //QuasarEngine::TestWeakPointer();
    //QuasarEngine::TestMemoryTrackerBalance();
    //QuasarEngine::BenchmarkPointerCreation();

    try
    {
        QuasarEngine::TestSimpleJobs();
        QuasarEngine::TestDependencies();
        QuasarEngine::TestStress();
        QuasarEngine::TestThreadPool();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception : " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nAll tests passed!\n";
	return 0;
}