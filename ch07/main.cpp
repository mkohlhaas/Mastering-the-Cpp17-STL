#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#define BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-private-field"

// The problem with volatile

namespace ex01
{
    volatile int &
    memory_mapped_register_x()
    {
        static int x = 0;
        return x;
    }

    volatile bool &
    memory_mapped_register_y()
    {
        static bool y = 0;
        return y;
    }

    void
    test()
    {
        volatile int  &x = memory_mapped_register_x();
        volatile bool &y = memory_mapped_register_y();

        int stack;

        stack = x;    // load
        y     = true; // store
        stack += x;   // load

        // would be the same without volatile - but with volatile compiler can't optimize
        stack = 2 * x; // load
        y     = true;  // store

        (void)stack;
    }
} // namespace ex01

// volatile is obsolete

namespace ex03
{
    // volatile accesses are not guaranteed to be atomic
    // volatile accesses are not guaranteed to be sequentially consistent

    // Global variables:
    int64_t x = 0;
    bool    y = false;

    void
    thread_A()
    {
        x = 0x42'00000042;
        y = true;
    }

    void
    thread_B()
    {
        if (x)
        {
            assert(x == 0x42'00000042);
        }
    }

    void
    thread_C()
    {
        if (y)
        {
            assert(x == 0x42'00000042);
        }
    }
} // namespace ex03

// Using std::atomic<T> for thread-safe accesses

namespace ex04
{
    // Global variables:
    // Think of std::atomic as a magical built-in family of types whose names just happen to contain angle brackets!
    //
    // This way of thinking about it is actually pretty useful, because it suggests - correctly - that std::atomic
    // is partly built into the compiler, and so the compiler will usually generate optimal code for atomic operations.
    std::atomic<int64_t> x = 0;
    std::atomic<bool>    y = false;

    void
    thread_A()
    {
        x = 0x42'00000042; // atomic!
        y = true;          // atomic!
    }

    void
    thread_B()
    {
        if (x)
        {
            // The assignment to x happens atomically.
            assert(x == 0x42'00000042);
        }
    }

    void
    thread_C()
    {
        if (y)
        {
            // The assignment to x "happens before" the
            // assignment to y, even from another thread's
            // point of view.
            assert(x == 0x42'00000042);
        }
    }
} // namespace ex04

namespace ex05
{
    void
    test()
    {

#if 0
        // There is a difference between objects of type std::atomic<T>
        // (which conceptually live "out there" in memory) and short-lived values of type T (which
        // conceptually live "right here," close at hand; for example, in CPU registers)
    
        std::atomic<int> a, b;
        a = b; // DOES NOT COMPILE! There is no copy-assignment operator for std::atomic<int>.
#endif
        // Memory-to-memory in an atomic operation isnt't supported by most hardware platforms

        // You have to explicitly say what you mean!
        std::atomic<int>       a = 0;
        const std::atomic<int> b = 0;

        int shortlived = b;          // atomic load
        a              = shortlived; // atomic store
    }
} // namespace ex05

namespace ex06
{
    void
    test()
    {
        std::atomic<int>       a = 0;
        const std::atomic<int> b = 0;

        // use of load() and store() is optional
        int shortlived = b.load(); // atomic load
        a.store(shortlived);       // atomic store

        // the same:
        a.store(b.load()); // But not recommended! Stick to a single atomic operation per source line!
    }
} // namespace ex06

// Doing complicated operations atomically

namespace ex08
{
    void
    operator*=(std::atomic<int> &, int)
    {
    }

    void
    test()
    {
        std::cout << "== namespace ex08 ==\n";

        std::atomic<int> a = 6;

        a *= 9; // This isn't allowed!
        assert(a == 6);

        // But this is:
        int expected, desired;
        do
        {
            std::cout << "compare and swap" << '\n';
            expected = a.load();
            desired  = expected * 9;
        } while (!a.compare_exchange_weak(expected, desired)); // atomic operation "compare and swap"
        // IF a HAS THE expected VALUE, GIVE IT THE desired VALUE!

        // The meaning of a.compare_exchange_weak(expected, desired) is that the processor
        // should look at a; and if its value is currently expected, then set its value to desired;
        // otherwise don't. The function call returns true if a was set to desired and false
        // otherwise.

        // At the end of this loop, a's value will
        // have been "atomically" multiplied by 9.
        assert(a == 54);
    }
} // namespace ex08

namespace ex09
{
    void
    test()
    {
        std::atomic<int> a = 6;

        // loading expected only once
        int expected = a.load();
        while (!a.compare_exchange_weak(expected, expected * 9))
        {
            // ... continue looping ...
        }
        assert(a == 54);

        // The dirty little secret of std::atomic is that most of the compound assignment operations
        // are implemented as compare-exchange loops just like this.
    }
} // namespace ex09

// Taking turns with std::mutex

namespace ex10
{
    void
    log(const char *message)
    {
        // If there is just one critical section that needs protection,
        // you can put the mutex in a function-scoped static variable.
        //
        // The static keyword here is very important! If we had omitted it, then m would have been
        // a plain old stack variable, and each thread that entered log would have received its own
        // distinct copy of m.
        static std::mutex m;
        {                  // braces not needed; just added to make critical section obvious
            m.lock();
            puts(message); // avoid interleaving messages on stdout
            m.unlock();
        }
    }
} // namespace ex10

namespace ex11
{
    static std::mutex m;

    void
    log1(const char *message)
    {
        m.lock();
        printf("LOG1: %s\n", message);
        m.unlock();
    }

    void
    log2(const char *message)
    {
        m.lock();
        printf("LOG2: %s\n", message);
        m.unlock();
    }
} // namespace ex11

namespace ex12
{
    // Try to eliminate the global mutex variable by creating a class type
    // and making the mutex object a member variable of that class.

    // Now messages printed by one Logger may interleave with messages printed by another
    // Logger, but concurrent accesses to the same Logger object will take locks on the same
    // m_mtx, which means they will block each other and nicely take turns entering the critical
    // functions log1 and log2, one at a time.
    struct Logger
    {
        std::mutex m_mtx;

        void
        log1(const char *message)
        {
            m_mtx.lock();
            printf("LOG1: %s\n", message);
            m_mtx.unlock();
        }

        void
        log2(const char *message)
        {
            m_mtx.lock();
            printf("LOG2: %s\n", message);
            m_mtx.unlock();
        }
    };
} // namespace ex12

// "Taking locks" the right way

namespace ex13
{
    // Avoiding lock leaks:
    // - You might take a lock on a particular mutex, and accidentally forget to write the code that frees it.
    // - You might have written that code, but due to an early return or an exception being thrown, the code
    //   never runs and the mutex remains locked!

    // How unique_lock could be implemented - not used:
    template <typename Mutex>
    class unique_lock
    {
      private:
        Mutex *m_mtx    = nullptr;
        bool   m_locked = false;

      public:
        constexpr unique_lock() noexcept = default;

        constexpr unique_lock(Mutex *p) noexcept : m_mtx(p)
        {
        }

        Mutex *
        mutex() const noexcept
        {
            return m_mtx;
        }

        bool
        owns_lock() const noexcept
        {
            return m_locked;
        }

        void
        lock()
        {
            m_mtx->lock();
            m_locked = true;
        }

        void
        unlock()
        {
            m_mtx->unlock();
            m_locked = false;
        }

        unique_lock(unique_lock &&rhs) noexcept
        {
            m_mtx    = std::exchange(rhs.m_mtx, nullptr);
            m_locked = std::exchange(rhs.m_locked, false);
        }

        unique_lock &
        operator=(unique_lock &&rhs)
        {
            if (m_locked)
            {
                unlock();
            }
            m_mtx    = std::exchange(rhs.m_mtx, nullptr);
            m_locked = std::exchange(rhs.m_locked, false);
            return *this;
        }

        ~unique_lock()
        {
            if (m_locked)
            {
                unlock();
            }
        }
    };

    void
    test()
    {
        std::mutex m;

        std::unique_lock<std::mutex> lk(m);
        assert(lk.owns_lock());

        auto lk2 = std::move(lk);
        assert(!lk.owns_lock());
        assert(lk2.owns_lock());

        lk2.unlock();
        assert(!lk2.owns_lock());
    }
} // namespace ex13

namespace ex14
{
    struct Lockbox
    {
        std::mutex m_mtx;
        int        m_value = 0;

        void
        locked_increment()
        {
            std::lock_guard<std::mutex> lk(m_mtx);
            m_value += 1;
        }

        void
        locked_decrement()
        {
            std::lock_guard lk(m_mtx); // C++17 only: CTAD
            m_value -= 1;
        }
    };

    void
    test()
    {
        Lockbox l;
        l.locked_increment();
        l.locked_decrement();
        assert(l.m_value == 0);
    }
} // namespace ex14

// Always associate a mutex with its controlled data

namespace ex15
{
    class StreamingAverage
    {
      private:
        double m_sum          = 0;
        int    m_count        = 0;
        double m_last_average = 0;

        std::mutex m_mtx;

      public:
        // Called from the single producer thread
        void
        add_value(double x)
        {
            std::lock_guard lk(m_mtx);
            m_sum += x;
            m_count += 1; // A
        }

        // Called from the single consumer thread
        double
        get_current_average()
        {
            std::lock_guard lk(m_mtx);
            m_last_average = m_sum / m_count; // B
            return m_last_average;
        }

        // Called from the single consumer thread
        double
        get_last_average() const
        {
            return m_last_average; // C
        }

        // Called from the single consumer thread
        double
        get_current_count() const
        {
            return m_count; // D
        }
    };
} // namespace ex15

namespace ex16
{
    class StreamingAverage
    {
        double m_sum          = 0;
        int    m_count        = 0;
        double m_last_average = 0;

        std::mutex m_sum_count_mtx; // careful naming of the mutex

        // ...
    };
} // namespace ex16

namespace ex17
{
    class StreamingAverage
    {
        // a better way is via a nested struct definition
        struct
        {
            double     sum   = 0;
            int        count = 0;
            std::mutex mtx;
        } m_guarded_sc;

        double m_last_average = 0;

        // ...
    };
} // namespace ex17

namespace ex18
{
    template <class Data>
    class Guarded
    {
      private:
        std::mutex m_mtx;
        Data       m_data;

        class Handle
        {
          private:
            std::unique_lock<std::mutex> m_lk;
            Data                        *m_ptr;

          public:
            Handle(std::unique_lock<std::mutex> lk, Data *p) : m_lk(std::move(lk)), m_ptr(p)
            {
            }

            auto
            operator->() const
            {
                return m_ptr;
            }
        };

      public:
        // Place the mutex in a class that allows access to its private members
        // only when the mutex is locked, by returning an RAII handle.
        Handle
        lock()
        {
            std::unique_lock lk(m_mtx);
            return Handle{std::move(lk), &m_data}; // unique_lock is movable; lock_guard is not
        }
    };

    class StreamingAverage
    {
        struct Guts
        {
            double m_sum   = 0;
            int    m_count = 0;
        };

        Guarded<Guts> m_sc;
        double        m_last_average = 0;

        // ...

        double
        get_current_average()
        {
            auto guts_handle = m_sc.lock();
            m_last_average   = guts_handle->m_sum / guts_handle->m_count;
            return m_last_average;
        }
    };
} // namespace ex18

namespace ex20
{
    class StreamingAverage
    {
        struct Guts
        {
            double m_sum   = 0;
            int    m_count = 0;
        };

        ::ex18::Guarded<Guts> m_sc;

        // Because of the two calls to m_sc.lock(), there is a gap between
        // the read of m_sum and the read of m_count. If the producer thread
        // calls add_value during this gap, we will compute an incorrect average.
        double
        get_sum()
        {
            return m_sc.lock()->m_sum;
        }

        int
        get_count()
        {
            return m_sc.lock()->m_count;
        }

        double
        get_current_average()
        {
            return get_sum() / get_count();
        }
    };
} // namespace ex20

namespace ex21
{
    class StreamingAverage
    {
        struct Guts
        {
            double m_sum   = 0;
            int    m_count = 0;
        };

        ::ex18::Guarded<Guts> m_sc;

        double
        get_sum()
        {
            return m_sc.lock()->m_sum; // LOCK 2
        }

        int
        get_count()
        {
            return m_sc.lock()->m_count;
        }

        double
        get_current_average()
        {
            // The line marked LOCK 1 causes the mutex to become locked;
            // then, on the line marked LOCK 2, we try to lock the mutex again.
            // -> Deadlock with oneself!
            auto h = m_sc.lock(); // LOCK 1
            return get_sum() / get_count();
        }
    };
} // namespace ex21

// Special-purpose mutex types

namespace ex22
{
    void
    test()
    {
        std::cout << "== namespace ex22 ==\n";

        using namespace std::literals;

        std::timed_mutex  m; // offers try_lock_for() and try_lock_until()
        std::atomic<bool> ready = false;

        std::thread thread_b([&]() {
            std::lock_guard lk(m);
            puts("Thread B got the lock.");
            ready = true;
            std::this_thread::sleep_for(100ms);
        });

        while (!ready)
        {
            puts("Thread A is waiting for thread B to launch.");
            std::this_thread::sleep_for(10ms);
        }

        while (!m.try_lock_for(10ms))
        {
            puts("Thread A spent 10ms trying to get the lock and failed.");
        }

        puts("Thread A finally got the lock!");
        m.unlock();

        thread_b.join();
    }
} // namespace ex22

namespace ex23
{
    void
    test()
    {
        std::cout << "== namespace ex23 ==\n";

        using namespace std::literals;

        auto count_ms = [](auto &&duration) -> int {
            using namespace std::chrono;
            return duration_cast<milliseconds>(duration).count();
        };

        std::timed_mutex  m1, m2;
        std::atomic<bool> ready = false;

        std::thread thread_b([&]() {
            std::this_thread::sleep_for(50ms);
            std::unique_lock lk1(m1);
            std::unique_lock lk2(m2);
            puts("Thread B got the two locks.");
            ready = true;
            std::this_thread::sleep_for(50ms);
            lk1.unlock();
            std::this_thread::sleep_for(50ms);
        });

        // Be careful never to write a polling loop that does not contain a sleep,
        // because in that case the compiler is within its rights to collapse all the
        // consecutive loads of ready down into a single load and a (necessarily
        // infinite) loop.
        while (!ready)
        {
            printf("Thread A is sleeping.\n");
            std::this_thread::sleep_for(10ms);
        }

        auto start_time = std::chrono::system_clock::now();
        auto deadline   = start_time + 200ms;

        bool got_m1     = m1.try_lock_until(deadline);
        auto elapsed_m1 = std::chrono::system_clock::now() - start_time;

        bool got_m2     = m2.try_lock_until(deadline);
        auto elapsed_m2 = std::chrono::system_clock::now() - start_time;

        if (got_m1)
        {
            printf("Thread A got the 1st lock after %dms.\n", count_ms(elapsed_m1));
            m1.unlock();
        }

        if (got_m2)
        {
            printf("Thread A got the 2nd lock after %dms.\n", count_ms(elapsed_m2));
            m2.unlock();
        }

        thread_b.join();
    }
} // namespace ex23

// Upgrading a read-write lock

namespace ex25
{
    // There are third-party libraries that attempt to solve this problem,
    // such as boost::thread::upgrade_lock.

    // The standard solution is that if you hold a read lock and want a
    // write lock, you must release your read lock and then go stand in line
    // for a write lock with everyone else
    template <class M>
    std::unique_lock<M>
    upgrade(std::shared_lock<M> lk)
    {
        lk.unlock();
        // Some other writer might sneak in here.
        return std::unique_lock<M>(*lk.mutex());
    }

    void
    test()
    {
        std::shared_mutex m;
        std::shared_lock  slk(m); // shared lock is a read-write lock (one writer - many reads; one or the other)

        auto ulk = upgrade(std::move(slk));
        assert(ulk.owns_lock());
    }
} // namespace ex25

// Downgrading a read-write lock

namespace ex26
{
    // If your architectural design calls for a read-write lock,
    // I strongly recommend using something like boost::thread::shared_mutex,
    // which comes "batteries included."

    template <class M>
    std::shared_lock<M>
    downgrade(std::unique_lock<M> lk)
    {
        lk.unlock();
        // Some other writer might sneak in here.
        return std::shared_lock<M>(*lk.mutex());
    }

    void
    test()
    {
        std::shared_mutex m;
        std::unique_lock  ulk(m);

        auto slk = downgrade(std::move(ulk));
        assert(slk.owns_lock());
    }
} // namespace ex26

// Waiting for a condition

// std::condition_variable is a synchronization primitive used with a std::mutex
// to block one or more threads until another thread both modifies a shared variable
// (the condition) and notifies the std::condition_variable.

namespace ex27
{
    bool prepped = false;

    void
    prep_work()
    {
        prepped = true;
    }

    void
    main_work()
    {
    }

    void
    test()
    {
        std::atomic<bool> ready = false;

        std::thread thread_b([&]() {
            prep_work();
            ready = true;
            main_work();
        });

        // Wait for thread B to be ready.
        // Wasteful polling loop on a std::atomic variable.
        while (!ready)
        {
            using namespace std::literals;
            std::this_thread::sleep_for(10ms);
        }

        // Now thread B has completed its prep work.
        assert(prepped);
        thread_b.join();
    }
} // namespace ex27

namespace ex28
{
    bool prepped = false;

    void
    prep_work()
    {
        prepped = true;
    }

    void
    main_work()
    {
    }

    void
    test()
    {
        using cond_var = std::condition_variable;

        bool       ready = false; // not atomic!
        std::mutex ready_mutex;
        cond_var   cv;

        std::thread thread_b([&]() {
            prep_work();
            {
                std::lock_guard lk(ready_mutex);
                ready = true;
            }
            cv.notify_one();
            main_work();
        });

        // Wait for thread B to be ready.
        {
            std::unique_lock lk(ready_mutex);
            cv.wait(lk, [&ready] { return ready; });
        }

        // Now thread B has completed its prep work.
        assert(prepped);
        thread_b.join();
    }
} // namespace ex28

// Manually fiddling with mutex locks and condition variables is
// almost as dangerous as fiddling with raw mutexes or raw pointers.

namespace ex29
{
    bool prepped = false;

    void
    prep_work()
    {
        prepped = true;
    }

    void
    main_work()
    {
    }

    void
    test()
    {
        // read-write lock version

        using cond_var = std::condition_variable_any;
        using sh_mutex = std::shared_mutex;

        bool     ready = false;
        sh_mutex ready_rwlock;
        cond_var cv;

        std::thread thread_b([&]() {
            prep_work();
            {
                std::lock_guard lk(ready_rwlock);
                ready = true;
            }
            cv.notify_one();
            main_work();
        });

        // Wait for thread B to be ready.
        {
            std::shared_lock lk(ready_rwlock);
            cv.wait(lk, [&ready] { return ready; });
        }

        // Now thread B has completed its prep work.
        assert(prepped);
        thread_b.join();
    }
} // namespace ex29

// Promises about futures

// We might say that a promise-future pair is like a directed, portable, one-shot wormhole. It's
// "directed" because you're allowed to shove data into only the "promise" side and retrieve
// data only via the "future" side. It's "portable" because if you own one end of the wormhole,
// you can move that end around and even move it between threads; you won't break the
// tunnel between the two ends. And it's "one-shot" because once you've shoved one piece of
// data into the "promise" end, you can't shove any more.

namespace ex30
{
    void
    test()
    {
        std::cout << "== namespace ex30 ==\n";

        using namespace std::literals;
        auto count_ms = [](auto &&duration) -> int {
            using namespace std::chrono;
            return duration_cast<milliseconds>(duration).count();
        };

        std::promise<int> p1, p2;
        std::future<int>  f1 = p1.get_future();
        std::future<int>  f2 = p2.get_future();

        // If the promise is satisfied first,
        // then f.get() will not block.
        p1.set_value(42);
        assert(f1.get() == 42);

        // If f.get() is called first, then it
        // will block until set_value() is called
        // from some other thread.
        std::thread t([&]() {
            std::this_thread::sleep_for(100ms);
            p2.set_value(43);
        });

        auto start_time = std::chrono::system_clock::now();
        assert(f2.get() == 43);

        auto elapsed = std::chrono::system_clock::now() - start_time;
        printf("f2.get() took %dms.\n", count_ms(elapsed));
        t.join();
    }
} // namespace ex30

namespace ex31
{
    bool prepped = false;

    void
    prep_work()
    {
        prepped = true;
    }

    void
    main_work()
    {
    }

    void
    test()
    {
        // very clean code

        std::promise<void> ready_p;
        std::future<void>  ready_f = ready_p.get_future(); // no data shoving; signal sending/receiving

        std::thread thread_b([&]() {
            prep_work();
            ready_p.set_value();                           // sending signal
            main_work();
        });

        ready_f.wait();                                    // wait for thread B to be ready; receiving signal

        assert(prepped);                                   // Now thread B has completed its prep work.
        thread_b.join();
    }
} // namespace ex31

namespace ex32
{
    template <class T = void>

    struct MyAllocator
    {
        using value_type = T;
        MyAllocator()    = default;

        template <class U>
        MyAllocator(const MyAllocator<U> &)
        {
        }

        T *
        allocate(size_t n)
        {
            return reinterpret_cast<T *>(new char[sizeof(T) * n]);
        }

        void
        deallocate(T *p, size_t)
        {
            delete[] reinterpret_cast<char *>(p);
        }

        template <class U>
        struct rebind
        {
            using other = MyAllocator<U>;
        };
    };

    MyAllocator() -> MyAllocator<void>;

    void
    test()
    {
        MyAllocator       myalloc{};
        std::promise<int> p(std::allocator_arg, myalloc);
        std::future<int>  f = p.get_future();

        (void)p;
        (void)f;
    }
} // namespace ex32

// Packaging up tasks for later

namespace ex33
{
    template <class T>
    class simple_packaged_task
    {
        std::function<T()> m_func;
        std::promise<T>    m_promise;

      public:
        template <typename F>
        simple_packaged_task(const F &f) : m_func(f)
        {
        }

        auto
        get_future()
        {
            return m_promise.get_future();
        }

        void
        operator()()
        {
            try
            {
                T result = m_func();
                m_promise.set_value(result);
            }
            catch (...)
            {
                // By using promises and futures, we can "teleport" exceptions across threads.
                m_promise.set_exception(std::current_exception()); // shove an exception through the wormhole
            }
        }
    };
} // namespace ex33

// The future of futures

namespace ex34
{
    using Data       = int;
    using Connection = double;

    Connection
    slowly_open_connection()
    {
        return 0;
    }

    Data
    slowly_get_data_from_disk()
    {
        return 1;
    }

    Data
    slowly_get_data_from_connection(Connection)
    {
        return 1;
    }

    template <typename T>
    auto
    pf()
    {
        std::promise<T> p;
        std::future<T>  f = p.get_future();
        return std::make_pair(std::move(p), std::move(f));
    }

    void
    test()
    {
        auto [p1, f1] = pf<Connection>();
        auto [p2, f2] = pf<Data>();
        auto [p3, f3] = pf<Data>();

        auto t1 = std::thread([p1 = std::move(p1)]() mutable {
            Connection conn = slowly_open_connection();
            p1.set_value(conn);
            // DANGER: what if slowly_open_connection throws?
            // We needed a try...catch statement - for every thread.
        });

        auto t2 = std::thread([p2 = std::move(p2)]() mutable {
            Data data = slowly_get_data_from_disk();
            p2.set_value(data);
        });

        auto t3 = std::thread([p3 = std::move(p3), f1 = std::move(f1)]() mutable {
            Data data = slowly_get_data_from_connection(f1.get());
            p3.set_value(data);
        });

        bool success = (f2.get() == f3.get());
        assert(success);

        t1.join();
        t2.join();
        t3.join();
    }
} // namespace ex34

namespace ex35
{
    using Data       = int;
    using Connection = double;

    Connection
    slowly_open_connection()
    {
        return 0;
    }

    Data
    slowly_get_data_from_disk()
    {
        return 1;
    }

    Data
    slowly_get_data_from_connection(Connection)
    {
        return 1;
    }

    void
    test()
    {
        // If the function of std::async returns a value or throws an exception,
        // it is stored in the shared state accessible through the std::future
        // that std::async returns to the caller.

        // Bad code!
        //
        // Every time you see a .get() in that code, you should think, "What a waste of a context
        // switch!"
        // And every time you see a thread being spawned (whether explicitly or via async),
        // you should think, "What a possibility for the operating system to run out of kernel threads
        // and for my program to start throwing unexpected exceptions from the constructor of std::thread!"

        // Something like this is already available in Boost; and Facebook's Folly library contains a
        // particularly robust and fully featured implementation even better than Boost's. While we
        // wait for C++20 to improve the situation, my advice is to use Folly if you can afford the
        // cognitive overhead of integrating it into your build system. The single advantage of
        // std::future is that it's standard; you'll be able to use it on just about any platform
        // without needing to worry about downloads, include paths, or licensing terms.

        auto f1 = std::async(slowly_open_connection);
        auto f2 = std::async(slowly_get_data_from_disk);
        auto f3 = std::async([f1 = std::move(f1)]() mutable {
            return slowly_get_data_from_connection(f1.get());
            // No more danger.
        });

        bool success = (f2.get() == f3.get());

        assert(success);
    }
} // namespace ex35

// Speaking of threads...

namespace ex37
{
    void
    test()
    {
        std::cout << "== namespace ex37 ==\n";

        using namespace std::literals;

        std::thread a([]() {
            puts("Thread A says hello ~0ms");
            std::this_thread::sleep_for(10ms);
            puts("Thread A says goodbye ~10ms");
        });

        std::thread b([]() {
            puts("Thread B says hello ~0ms");
            std::this_thread::sleep_for(20ms);
            puts("Thread B says goodbye ~20ms");
        });

        puts("The main thread says hello ~0ms");
        a.join();   // waits for thread A
        b.detach(); // doesn't wait for thread B
        puts("The main thread says goodbye ~10ms");
    }
} // namespace ex37

// Identifying individual threads and the current thread

namespace ex41
{
    std::string
    to_string(std::thread::id id)
    {
        std::ostringstream o;
        o << id;
        return o.str();
    }

    void
    test()
    {
        std::cout << "== namespace ex41 ==\n";

        using namespace std::literals;

        std::mutex               ready;
        std::unique_lock         lk(ready);
        std::vector<std::thread> threads;
        std::vector<std::thread> others;

        auto task = [&]() {
            // Block here until the main thread is ready.
            (void)std::lock_guard(ready);
            // Now go. Find my thread-id in the vector.
            auto my_id = std::this_thread::get_id();
            auto iter  = std::find_if(threads.begin(),                                            //
                                      threads.end(),                                              //
                                      [=](const std::thread &t) { return t.get_id() == my_id; }); //

            printf("Thread %s %s in the list.\n", to_string(my_id).c_str(), iter != threads.end() ? "is" : "is not");
        };

        for (int i = 0; i < 10; ++i)
        {
            std::thread t(task);
            if (i % 2)
            {
                threads.push_back(std::move(t));
            }
            else
            {
                others.push_back(std::move(t));
            }
        }

        // Let all the threads run.
        ready.unlock();

        // Join all the threads.
        for (std::thread &t : threads)
        {
            t.join();
        }

        for (std::thread &t : others)
        {
            t.join();
        }
    }
} // namespace ex41

// Thread exhaustion and std::async

namespace ex38
{
    template <typename F>
    auto
    async(F &&func)
    {
        using ResultType  = std::invoke_result_t<std::decay_t<F>>;
        using PromiseType = std::promise<ResultType>;
        using FutureType  = std::future<ResultType>;

        PromiseType promise;
        FutureType  future = promise.get_future();

        auto t = std::thread([func = std::forward<F>(func), promise = std::move(promise)]() mutable {
            try
            {
                ResultType result = func();
                promise.set_value(result);
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        });

        // This special behavior is not implementable
        // outside of the library, but async does do it.
        // future.on_destruction([t = std::move(t)]() {
        //     t.join();
        // });
        return future;
    }

    void
    test()
    {
        auto p = std::make_unique<int>(42);
        async([p = std::move(p)]() { return *p; });
    }
} // namespace ex38

namespace ex39
{
    template <class F>
    void
    fire_and_forget_wrong(const F &f)
    {
        // WRONG! Runs f in another thread, but blocks anyway.
        std::async(f);
    }

    template <class F>
    void
    fire_and_forget_better(const F &f)
    {
        // BETTER! Launches f in another thread without blocking.
        std::thread(f).detach();
    }
} // namespace ex39

namespace ex40
{
    int
    test()
    {
        int  i      = 0;
        auto future = std::async([&]() { i += 1; });
        // suppose we do not call f.wait() here
        return i;
    }
} // namespace ex40

// Building your own thread pool

namespace ex43
{
    class ThreadPool
    {
      private:
        using UniqueFunction = std::packaged_task<void()>; // folly::Function could be an alternative
        using vec_threads    = std::vector<std::thread>;
        using cond_var       = std::condition_variable;
        using queue_funcs    = std::queue<UniqueFunction>;

        struct
        {
            std::mutex  mtx;
            queue_funcs work_queue;
            bool        aborting = false;
        } m_state;

        vec_threads m_workers;
        cond_var    m_cv;

      public:
        ThreadPool(int size)
        {
            for (int i = 0; i < size; ++i)
            {
                m_workers.emplace_back([this]() { worker_loop(); });
            }
        }

        ~ThreadPool()
        {
            {
                std::lock_guard lk(m_state.mtx);
                m_state.aborting = true;
            }

            m_cv.notify_all();

            for (std::thread &t : m_workers)
            {
                t.join();
            }
        }

        void
        enqueue_task(UniqueFunction task)
        {
            {
                std::lock_guard lk(m_state.mtx);
                m_state.work_queue.push(std::move(task));
            }
            m_cv.notify_one();
        }

      private:
        void
        worker_loop()
        {
            while (true)
            {
                std::unique_lock lk(m_state.mtx);

                while (m_state.work_queue.empty() && !m_state.aborting)
                {
                    m_cv.wait(lk);
                }

                if (m_state.aborting)
                {
                    break;
                }

                // Pop the next task, while still under the lock.
                assert(!m_state.work_queue.empty());
                UniqueFunction task = std::move(m_state.work_queue.front());
                m_state.work_queue.pop();

                // Rule: Never call a user-provided callback while holding a mutex lock!
                // Would be a recipe for dead-lock!
                lk.unlock();

                // Actually run the task. This might take a while.
                // When we're done with this task, go get another.
                task();
            }
        }

      public:
        template <class F>
        auto
        async(F &&func)
        {
            using ResultType = std::invoke_result_t<std::decay_t<F>>;

            std::packaged_task<ResultType()> pt(std::forward<F>(func));
            std::future<ResultType>          future = pt.get_future();

            UniqueFunction task([pt = std::move(pt)]() mutable { pt(); });

            enqueue_task(std::move(task));

            // Give the user a future for retrieving the result.
            return future;
        }
    };

    void
    test()
    {
        using vec_future_ints = std::vector<std::future<int>>;

        std::atomic<int> sum(0);
        ThreadPool       tp(4);         // 4 kernel threads
        vec_future_ints  futures;

        for (int i = 0; i < 60000; ++i) // creating 60,000 tasks
        {
            auto f = tp.async([i, &sum]() {
                sum += i;
                return i;
            });
            futures.push_back(std::move(f));
        }
        std::cout << sum << '\n';

        assert(futures[42].get() == 42);
        assert(903 <= sum && sum <= 1799970000);
    }

    void
    test2()
    {
        std::cout << "== namespace ex43 ==\n";

        using namespace std::literals;

        std::future<int> f6;
        if (ThreadPool tp(4); true)
        {
            auto f1 = tp.async([]() {
                std::this_thread::sleep_for(10ms);
                return 1;
            });

            auto f2 = tp.async([]() {
                std::this_thread::sleep_for(20ms);
                return 2;
            });

            auto f3 = tp.async([]() {
                std::this_thread::sleep_for(30ms);
                return 3;
            });

            auto f4 = tp.async([]() {
                std::this_thread::sleep_for(40ms);
                return 4;
            });

            auto f5 = tp.async([]() {
                std::this_thread::sleep_for(50ms);
                return 5;
            });

            tp.async([] { std::this_thread::sleep_for(100ms); });
            tp.async([] { std::this_thread::sleep_for(100ms); });
            tp.async([] { std::this_thread::sleep_for(100ms); });
            tp.async([] { std::this_thread::sleep_for(100ms); });
            f6 = tp.async([]() {
                std::this_thread::sleep_for(60ms);
                return 6;
            });
            assert(f4.get() == 4);
        }
        puts("Done!");

        try
        {
            f6.get();
            assert(false);
        }
        catch (const std::future_error &ex)
        {
            assert(ex.code() == std::future_errc::broken_promise);
        }
    }
} // namespace ex43

// Improving our thread pool's performance

// Of course, there also exists professionally written thread-pool classes.
// Boost.Asio containsone, for example, and Asio is on track to be brought
// into the standard perhaps in C++20.

// namespace ex50
// {
//     using namespace boost::asio;
//
//     class ThreadPool
//     {
//       private:
//         boost::thread_group     m_workers;
//         boost::asio::io_context m_io;
//
//         // boost::asio::io_context::work m_work;
//
//       public:
//         ThreadPool(int size) // : m_work(m_io)
//         {
//             for (int i = 0; i < size; ++i)
//             {
//                 m_workers.create_thread([&]() { m_io.run(); });
//             }
//         }
//
//         template <class F>
//         void
//         enqueue_task(F &&func)
//         {
//             m_io.post(std::forward<F>(func));
//         }
//
//         ~ThreadPool()
//         {
//             m_io.stop();
//             m_workers.join_all();
//         }
//     };
//
//     template <class F>
//     auto
//     tp_async(ThreadPool &tp, F &&func)
//     {
//         using ResultType = std::invoke_result_t<std::decay_t<F>>;
//
//         std::packaged_task<ResultType()> pt(std::forward<F>(func));
//         std::future<ResultType>          future = pt.get_future();
//
//         tp.enqueue_task([pt = std::move(pt)]() mutable { pt(); });
//
//         // Give the user a future for retrieving the result.
//         return future;
//     }
//
//     void
//     test()
//     {
//         std::atomic<int> sum(0);
//         ThreadPool       tp(4);
//
//         std::vector<std::future<int>> futures;
//
//         for (int i = 0; i < 60000; ++i)
//         {
//             auto f = tp_async(tp, [i, &sum]() {
//                 sum += i;
//                 return i;
//             });
//             futures.push_back(std::move(f));
//         }
//
//         assert(futures[42].get() == 42);
//         assert(903 <= sum && sum <= 1799970000);
//     }
//
//     void
//     test2()
//     {
//         using namespace std::literals;
//
//         std::future<int> f6;
//
//         if (ThreadPool tp(4); true)
//         {
//             auto f1 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(10ms);
//                 return 1;
//             });
//
//             auto f2 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(20ms);
//                 return 2;
//             });
//
//             auto f3 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(30ms);
//                 return 3;
//             });
//
//             auto f4 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(40ms);
//                 return 4;
//             });
//
//             auto f5 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(50ms);
//                 return 5;
//             });
//
//             tp_async(tp, [] { std::this_thread::sleep_for(100ms); });
//             tp_async(tp, [] { std::this_thread::sleep_for(100ms); });
//             tp_async(tp, [] { std::this_thread::sleep_for(100ms); });
//             tp_async(tp, [] { std::this_thread::sleep_for(100ms); });
//             f6 = tp_async(tp, []() {
//                 std::this_thread::sleep_for(60ms);
//                 return 6;
//             });
//
//             assert(f4.get() == 4);
//         }
//         puts("Done!");
//
//         try
//         {
//             f6.get();
//             assert(false);
//         }
//         catch (const std::future_error &ex)
//         {
//             assert(ex.code() == std::future_errc::broken_promise);
//         }
//     }
// } // namespace ex50

int
main()
{
    ex01::test();
    ex05::test();
    ex06::test();
    ex08::test();
    ex09::test();
    ex13::test();
    ex14::test();
    ex22::test();
    ex23::test();
    ex25::test();
    ex26::test();
    ex27::test();
    ex28::test();
    ex29::test();
    ex30::test();
    ex31::test();
    ex32::test();
    ex34::test();
    ex35::test();
    ex37::test();

    using namespace std::literals;
    std::this_thread::sleep_for(20ms);

    ex40::test();
    ex41::test();
    ex43::test();
    ex43::test2();

    // ex50::test();
    // ex50::test2();
}
