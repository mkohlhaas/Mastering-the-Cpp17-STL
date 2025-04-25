#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>

// Automatically managing memory with std::unique_ptr<T>

namespace ex01
{
    namespace std
    {
        using ::std::exchange;

        // When the unique_ptr is adjusted to point elsewhere, or destroyed, the raw pointer will be freed correctly.
        template <typename T>
        class unique_ptr
        {
          private:
            T *m_ptr = nullptr;

          public:
            constexpr unique_ptr() noexcept = default;

            constexpr unique_ptr(T *p) noexcept : m_ptr(p)
            {
            }

            T *
            get() const noexcept
            {
                return m_ptr;
            }

            operator bool() const noexcept
            {
                return bool(get());
            }

            T &
            operator*() const noexcept
            {
                return *get();
            }

            T *
            operator->() const noexcept
            {
                return get();
            }

            // set internal pointer to p and delete old pointer
            // only direct delete operation
            void
            reset(T *p = nullptr) noexcept
            {
                T *old_p = std::exchange(m_ptr, p); //  std::exchange: pass in a new value, get out the old value
                delete old_p;
            }

            // returns internal pointer and sets internal pointer to nullptr
            T *
            release() noexcept
            {
                return std::exchange(m_ptr, nullptr);
            }

            // copy constructor
            // move rhs to this and release rhs
            unique_ptr(unique_ptr &&rhs) noexcept
            {
                this->reset(rhs.release());
            }

            // move asssignment
            // move rhs to this and delete rhs
            unique_ptr &
            operator=(unique_ptr &&rhs) noexcept
            {
                reset(rhs.release());
                return *this;
            }

            ~unique_ptr()
            {
                reset();
            }
        };
    } // namespace std

    namespace std
    {
        using ::std::forward; // forwards lvalues as either lvalues or as rvalues, depending on T. Perfect forwarding.

        // One little helper function, so as we never have to touch raw pointers with our hands.
        // The only way you'll get the full benefit of unique_ptr is if you make sure that whenever
        // you allocate a resource, you also initialize a unique_ptr to manage it.
        template <typename T, typename... Args>
        unique_ptr<T>
        make_unique(Args &&...args)
        {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    } // namespace std

    // Widget declarations
    struct Widget
    {
        virtual ~Widget();
    };

    struct WidgetImpl : Widget
    {
        WidgetImpl(int size);
    };

    struct WidgetHolder
    {
        void take_ownership_of(Widget *) noexcept;
    };

    void use(WidgetHolder &);

    // Widget implementations
    Widget::~Widget()
    {
    }

    WidgetImpl::WidgetImpl(int)
    {
    }

    void
    WidgetHolder::take_ownership_of(Widget *w) noexcept
    {
        delete w;
    }

    void
    use(WidgetHolder &)
    {
    }

    // Tests

    // old-style (obsolete) code
    void
    test1()
    {
        Widget       *w = new WidgetImpl(30);
        WidgetHolder *wh;

        try
        {
            wh = new WidgetHolder();
        }
        catch (...)
        {
            delete w;
            throw;
        }

        wh->take_ownership_of(w);
        try
        {
            use(*wh);
        }
        catch (...)
        {
            delete wh;
            throw;
        }
        delete wh;
    }

    // modern-style code
    void
    test2()
    {
        auto w  = std::make_unique<WidgetImpl>(30);
        auto wh = std::make_unique<WidgetHolder>();
        wh->take_ownership_of(w.release());
        use(*wh);
    }

    // Why C++ doesn't have the finally keyword
    //
    // C++ doesn't have the finally keyword, and shows no signs that it will ever enter the
    // language. This is simply due to a philosophical difference between C++ and other programming
    // languages: the C++ philosophy is that if you're concerned with enforcing some invariant - such
    // as "this pointer shall always be freed at the end of this block, no matter how we get there" -
    // then you shouldn't ever be writing explicit code, because then there's always a chance for you
    // to write it wrong, and then you'll have bugs.
    //
    // If you have some invariant that you want to enforce, then the right place to enforce it is in the
    // type system, using constructors, destructors, and other special member functions--the tools
    // of RAII. Then, you can ensure that any possible use of your new type preserves its invariants -
    // such as "the underlying pointer shall be freed whenever it's no longer held by an object of
    // this type" - and when you go to write your business logic, you won't have to write anything
    // explicitly; the code will look simple and yet always - provably - have correct behavior.
    // So if you find yourself writing code that looks like the preceding example, or if you find
    // yourself wishing you could just write `finally`, stop and think: "Should I be using
    // unique_ptr for this?" or "Should I write an RAII class type for this?"

} // namespace ex01

namespace ex07
{
    void
    test()
    {
        using ex01::Widget;
        using ex01::WidgetImpl;

        // While it is possible to use unique_ptr without using make_unique, you should not do it!
        using Widget_ptr = std::unique_ptr<Widget>;
        Widget_ptr bad(new WidgetImpl(30));
        bad.reset(new WidgetImpl(40));

        Widget_ptr good = std::make_unique<WidgetImpl>(30);
        good            = std::make_unique<WidgetImpl>(40);
    }
} // namespace ex07

// Customizing the deletion callback

namespace ex10
{
    struct fcloser
    {
        void
        operator()(FILE *fp) const
        {
            fclose(fp);
        }

        static auto
        open(const char *name, const char *mode)
        {
            // `make_unique` cannot specify a custom deleter.
            // The whole point of make_unique is to encapsulate the notion of
            // "use new to create a T from given constructor arguments and use delete to destroy it".

            // If you wanted a custom deleter, you would also have to specify how to create the object,
            // and then there would be nothing more gained from having the emplacing maker function.

            // Incidentally, notice that the destructor of std::unique_ptr is carefully written so that it
            // guarantees never to call your callback with a null pointer. This is absolutely critical in the
            // preceding example, because fclose(NULL) is a special case that means "close all open file
            // handles in the current process"--which is never what you wanted to do!
            return std::unique_ptr<FILE, fcloser>(fopen(name, mode));
        }
    };

    void
    use(FILE *)
    {
        throw "dummy throw";
    }

    void
    test()
    {
        // Observe also that std::make_unique<T>() only ever takes one template type parameter;
        // there is no std::make_unique<T,D>(). BUT THE RULE TO AVOID TOUCHING RAW POINTERS WITH
        // YOUR HANDS is still a good one; that's why this example wraps the fopen and
        // unique_ptr construction into a small reusable helper function fcloser::open, rather
        // than inlining the call to fopen into the body of test.

        try
        {
            // auto f = fopen("test.txt", "r"); // wrong!

            auto f = fcloser::open("test.txt", "r");
            use(f.get());
        }
        catch (...)
        {
            // f will be closed even when use() throws
        }
    }
} // namespace ex10

// Managing arrays with std::unique_ptr<T[]>
//
// Fortunately, as of C++11, std::unique_ptr<T[]> exists and does the right thing in this
// case (by virtue of the fact that std::default_delete<T[]> also exists and does the right
// thing, which is to call operator delete[]).

// Reference counting with std::shared_ptr<T>

namespace ex11
{
    // Just as unique_ptr has make_unique, the standard library provides shared_ptr with
    // make_shared so that you NEVER HAVE TO TOUCH RAW POINTERS WITH YOUR HANDS. The other
    // advantage of using std::make_shared<T>(args) to allocate shared objects is that
    // transferring ownership into a shared_ptr requires allocating additional memory for the
    // control block. When you call make_shared, THE LIBRARY IS PERMITTED TO ALLOCATE A SINGLE
    // CHUNK OF MEMORY that's big enough for both the control block and your T object, in one
    // allocation.

    struct X
    {
    };

    void
    test()
    {
        std::shared_ptr<X> pa, pb, pc;

        // use-count always starts at 1
        pa = std::make_shared<X>();
        assert(pa.use_count() == 1);
        assert(pb.use_count() == 0);
        assert(pc.use_count() == 0);

        // make a copy of the pointer
        pb = pa;
        assert(pa.use_count() == 2);
        assert(pb.use_count() == 2);
        assert(pc.use_count() == 0);

        // moving the pointer from pa to pc
        pc = std::move(pa);
        assert(pa == nullptr);
        assert(pa.use_count() == 0);
        assert(pb.use_count() == 2); // pb still at 2
        assert(pc.use_count() == 2);

        // decrement the use-count back to 1
        pb = nullptr;
        assert(pa.use_count() == 0);
        assert(pb.use_count() == 0);
        assert(pc.use_count() == 1); // pc is decremented to 1;pointer to X will be deleted only once
    }
} // namespace ex11

namespace ex12
{
    struct Super
    {
        int first, second;

        Super(int a, int b) : first(a), second(b)
        {
        }

        ~Super()
        {
            puts("destroying Super");
        }
    };

    auto
    get_second()
    {
        // Once ownership has been transferred into the shared_ptr system, the
        // responsibility for remembering how to free a managed resource rests entirely on the
        // shoulders of the control block. It isn't necessary for your code to deal in shared_ptr<T>
        // just because the underlying managed object happens to be of type T.

        auto p = std::make_shared<Super>(4, 2);
        // return std::make_shared<int>(2);         // prints: destroying Super           accesssing Super::second
        return std::shared_ptr<int>(p, &p->second); // prints: accesssing Super::second   destroying Super
    }

    void
    test()
    {
        std::shared_ptr<int> q = get_second();
        puts("accessing Super::second");
        assert(*q == 2);
    }
} // namespace ex12

// Don't double-manage!

namespace ex13
{
    struct X
    {
    };

    void
    test()
    {
        std::shared_ptr<X> pa, pb, pc;

        // use-count always starts at 1
        pa = std::make_shared<X>();
        assert(pa.use_count() == 1);
        assert(pb.use_count() == 0);
        assert(pc.use_count() == 0);

        // make a copy of the pointer; use-count is now 2
        pb = pa;
        assert(pa.use_count() == 2);
        assert(pb.use_count() == 2);
        assert(pc.use_count() == 0);

        // Give the same pointer to shared_ptr again, which tells shared_ptr to manage it - twice!
        // Will crash program!
        //
        // pc = std::shared_ptr<X>(pb.get()); // WRONG!
        //
        // Remember that your goal should be never to touch raw pointers with your hands! The place
        // where this code goes wrong is the very first time it calls pb.get() to fetch the raw pointer
        // out of shared_ptr.

        // It would have been correct to call the aliasing constructor here, pc =
        // std::shared_ptr<X>(pb, pb.get()), but that would have had the same effect as a
        // simple assignment pc = pb. So another general rule we can state is: if you have to use the
        // word shared_ptr explicitly in your code, you're doing something out of the ordinary--and
        // perhaps dangerous. Without naming shared_ptr in your code, you can already allocate
        // and manage heap objects (using std::make_shared) and manipulate a managed object's
        // use-count by creating and destroying copies of the pointer (using auto to declare variables
        // as you need them). The one place this rule definitely breaks down is when you sometimes
        // need to declare a class data member of type shared_ptr<T>; you generally can't do that
        // without writing the name of the type!

        // assert(pa.use_count() == 2);
        // assert(pb.use_count() == 2);
        // assert(pc.use_count() == 0);

        // pc's use-count drops to zero and shared_ptr calls "delete" on the X object
        pc = nullptr;
        assert(pa.use_count() == 2);
        assert(pb.use_count() == 2);
        assert(pc.use_count() == 0);

        // *pb; // accessing the freed object yields undefined behavior; program crashes
    }
} // namespace ex13

// Holding nullable handles with weak_ptr

namespace ex14
{
    struct DangerousWatcher
    {
        int *m_ptr = nullptr;

        void
        watch(const std::shared_ptr<int> &p)
        {
            m_ptr = p.get();
        }

        int
        current_value() const
        {
            // By now, *m_ptr might have been deallocated!
            return *m_ptr;
        }
    };
} // namespace ex14

namespace ex15
{
    struct NotReallyAWatcher
    {
        std::shared_ptr<int> m_ptr;

        void
        watch(const std::shared_ptr<int> &p)
        {
            m_ptr = p;
        }

        int
        current_value() const
        {
            // Now *m_ptr cannot ever be deallocated; our
            // mere existence is keeping *m_ptr alive!
            return *m_ptr;

            // What we really want is a non-owning reference that is nevertheless aware of the
            // shared_ptr system for managing memory, and is able to query the control block and find
            // out whether the referenced object still exists.
        }
    };
} // namespace ex15

namespace ex16
{
    struct CorrectWatcher
    {
        // pointer to an object without actually expressing ownership of that object

        // We don't want a non-owning reference; what we want is a ticket that we can exchange at some future date for
        // an owning reference. The standard library provides this "ticket for a shared_ptr" under the name
        // std::weak_ptr<T>. (It's called "weak" in opposition to the "strong" owning references of
        // shared_ptr.)
        std::weak_ptr<int> m_ptr;

        // The only two operations you need to know with weak_ptr are that you can:
        // - construct a weak_ptr<T> from a shared_ptr<T> (shared -> weak) [1]
        // - construct a shared_ptr<T> from a weak_ptr<T> by calling wptr.lock() (weak -> shared) [2].
        //
        // If the weak_ptr has expired, you'll get back a null shared_ptr.
        void
        watch(const std::shared_ptr<int> &p)
        {
            m_ptr = std::weak_ptr<int>(p); // construct a weak_ptr<T> from a shared_ptr<T> [1]
        }

        int
        current_value() const
        {
            // lock() creates a new std::shared_ptr that shares ownership of the managed object.
            // Now we can safely ask whether *m_ptr has been deallocated or not.
            if (auto p = m_ptr.lock()) // construct a shared_ptr<T> from a weak_ptr<T> by calling wptr.lock() [2]
            {
                return *p;
            }
            else
            {
                throw "It has no value; it's been deallocated!";
            }
        }
    };
} // namespace ex16

// Talking about oneself with std::enable_shared_from_this

namespace ex17
{
    using std::shared_ptr;
    using std::weak_ptr;

    template <class T>
    class enable_shared_from_this
    {
      private:
        weak_ptr<T> m_weak;

        // The constructor of shared_ptr includes some lines of code to
        // detect whether T publicly inherits from enable_shared_from_this<T>,
        // and, if so, to set its m_weak member through some hidden back door.

      public:
        // Don't do anything when copy constructing and assigning.
        // Default constructor would copy m_weak member variable!
        enable_shared_from_this(const enable_shared_from_this &)
        {
        }

        enable_shared_from_this &
        operator=(const enable_shared_from_this &)
        {
        }

        shared_ptr<T>
        shared_from_this() const
        {
            return shared_ptr<T>(m_weak);
        }
    };
} // namespace ex17

namespace ex18
{
    static int caught = 0;

    void
    puts(const char *msg)
    {
        std::cout << msg << '\n';
        caught += 1;
    }

    // Inheritance from std::enable_shared_from_this must be public and unambiguous.
    // CRTP
    struct Widget : std::enable_shared_from_this<Widget>
    {
        template <class F>
        void
        call_on_me(const F &f)
        {
            f(this->shared_from_this());
        }
    };

    void
    test1()
    {
        {
            // Shared pointer to Widget.
            auto shared_widget = std::make_shared<Widget>();
            assert(shared_widget.use_count() == 1);

            shared_widget->call_on_me([](auto shared_widget) {
                std::cout << "Calling myself!\n";
                assert(shared_widget.use_count() == 2);
            });
        }

        {
            // Not using a shared pointer to Widget.
            Widget w;
            try
            {
                w.call_on_me([](auto) {}); // fails
            }
            catch (const std::bad_weak_ptr &)
            {
                puts("Caught!");           // increases `caught`
            }
        }
    }

    void
    test()
    {
        caught = 0;
        test1();
        assert(caught == 1);
    }
} // namespace ex18

// The Curiously Recurring Template Pattern (CRTP)

// The pattern of "X inherits from A<X>" - X: A<X> - is known as the
// Curiously Recurring Template Pattern, or CRTP for short.

namespace ex19
{
    template <class Derived>
    class addable
    {
      public:
        auto
        operator+(const Derived &rhs) const
        {
            Derived lhs = static_cast<const Derived &>(*this);
            lhs += rhs; // calling += in Derived
            return lhs;
        }
    };

    static int copies, moves;

    struct D1 : addable<D1>
    {
        D1() = default;

        D1(const D1 &)
        {
            copies += 1;
        }

        D1(D1 &&)
        {
            moves += 1;
        }

        auto &
        operator+=(const D1 &)
        {
            return *this;
        }
    };

    // no moves, only copies
    void
    test()
    {
        D1 a, b;

        copies = moves = 0;
        D1 r1{a + b};
        assert(copies == 1 && moves == 0);

        copies = moves = 0;
        D1 r2          = std::move(a) + b;
        assert(copies == 1 && moves == 0);

        copies = moves = 0;
        D1 r3          = a + std::move(b);
        assert(copies == 1 && moves == 0);

        copies = moves = 0;
        D1 r4          = std::move(a) + std::move(b);
        assert(copies == 1 && moves == 0);

        // unused
        (void)r1;
        (void)r2;
        (void)r3;
        (void)r4;
    }
} // namespace ex19

// Barton-Nackman trick

namespace ex20
{
    template <class Derived>
    class addable
    {
      public:
        // Barton-Nackman trick: free friend function (it's not a member function)
        friend auto
        operator+(Derived lhs, const Derived &rhs)
        {
            lhs += rhs;
            return lhs;
        }
    };

    static int copies, moves;

    struct D1 : addable<D1>
    {
        D1() = default;

        D1(const D1 &)
        {
            copies += 1;
        }

        D1(D1 &&)
        {
            moves += 1;
        }

        auto &
        operator+=(const D1 &)
        {
            return *this;
        }
    };

    void
    test()
    {
        D1 a, b;
        copies = moves = 0;
        D1 r1{a + b};
        assert(copies == 1 && moves == 1);

        copies = moves = 0;
        D1 r2          = std::move(a) + b;
        assert(copies == 0 && moves == 2);

        copies = moves = 0;
        D1 r3          = a + std::move(b);
        assert(copies == 1 && moves == 1);

        copies = moves = 0;
        D1 r4          = std::move(a) + std::move(b);
        assert(copies == 0 && moves == 2);

        // unused
        (void)r1;
        (void)r2;
        (void)r3;
        (void)r4;
    }
} // namespace ex20

// A final warning
//
// The mini-ecosystem of shared_ptr, weak_ptr, and enable_shared_from_this is one of
// the coolest parts of modern C++; it can give your code the safety of a garbage-collected
// language while preserving the speed and deterministic destruction that have always
// characterized C++.
//
// However, be careful not to abuse shared_ptr!
//
// MOST OF YOUR C++ CODE SHOULDN'T BE USING SHARED_PTR AT ALL, because you shouldn't be
// sharing the ownership of heap-allocated objects. Your first preference should always
// be to avoid heap-allocation altogether (by using value semantics); your second
// preference should be to make sure each heap-allocated object has a unique owner
// (by using std::unique_ptr<T>); and only if both of those are really impossible
// should you consider use of shared ownership and std::shared_ptr<T>.
//
// Order of preference:
// - avoid heap allocations; allocate on the stack (using value semantics)
// - use unique_ptr
// - use shared_ptr only if really necessary

// Denoting un-special-ness with observer_ptr<T>

namespace ex21
{
    // unique_ptr shows intention
    // unique_ptr<T> is a vocabulary type for expressing ownership transfer

    class Widget;

    void                    consumer(std::unique_ptr<Widget> p); // consumes Widget; we pass ownership to consumer()
    std::unique_ptr<Widget> producer(void);                      // caller will be owner of produced Widget
} // namespace ex21

namespace ex22
{
    // raw pointer shows no intention
    class Widget;

    void ambiguous(Widget *p); // consumer or producer???
} // namespace ex22

namespace ex23
{
    class Widget;

    // observer_ptr is nothing but a wrapped-up non-owning raw pointer, but shows intention -> read-only
    // "the world's dumbest smart pointer"
    template <typename T>
    class observer_ptr
    {
        T *m_ptr = nullptr;

      public:
        constexpr observer_ptr() noexcept = default;

        constexpr observer_ptr(T *p) noexcept : m_ptr(p)
        {
        }

        T *
        get() const noexcept
        {
            return m_ptr;
        }

        operator bool() const noexcept
        {
            return bool(get());
        }

        T &
        operator*() const noexcept
        {
            return *get();
        }

        T *
        operator->() const noexcept
        {
            return get();
        }
    };

    // observe() is "opting out" of the whole ownership debate.
    // observe() merely observes its argument; read-only.
    void observe(observer_ptr<Widget> p);

    // There are also many knowledgeable people who would say that `T*` should
    // be the vocabulary type for non-owning pointers.

} // namespace ex23

int
main()
{
    ex01::test1();
    ex01::test2();
    ex10::test();
    ex11::test();
    ex12::test();
    ex13::test(); // has undefined behavior; crashes
    ex18::test();
    ex19::test(); // crashes
    ex20::test();
}
