#include <cassert>
#include <cstdio>
#include <memory>
#include <utility>

namespace ex01
{
    namespace std
    {
        using ::std::exchange;
        using ::std::forward;

        template <typename T>
        class unique_ptr
        {
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

            void
            reset(T *p = nullptr) noexcept
            {
                T *old_p = std::exchange(m_ptr, p);
                delete old_p;
            }

            T *
            release() noexcept
            {
                return std::exchange(m_ptr, nullptr);
            }

            unique_ptr(unique_ptr &&rhs) noexcept
            {
                this->reset(rhs.release());
            }

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
        template <typename T, typename... Args>
        unique_ptr<T>
        make_unique(Args &&...args)
        {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    } // namespace std

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

    void
    test2()
    {
        auto w  = std::make_unique<WidgetImpl>(30);
        auto wh = std::make_unique<WidgetHolder>();
        wh->take_ownership_of(w.release());
        use(*wh);
    }

    void
    test3()
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
} // namespace ex01

namespace ex07
{
    void
    test()
    {
        using ex01::Widget;
        using ex01::WidgetImpl;

        using Widget_ptr = std::unique_ptr<Widget>;

        Widget_ptr bad(new WidgetImpl(30));
        bad.reset(new WidgetImpl(40));

        Widget_ptr good = std::make_unique<WidgetImpl>(30);
        good            = std::make_unique<WidgetImpl>(40);
    }
} // namespace ex07

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
            return std::unique_ptr<FILE, fcloser>(fopen(name, mode));
        }
    };

    void use(FILE *);

    void
    test()
    {
        auto f = fcloser::open("test.txt", "r");
        use(f.get());
        // f will be closed even if use() throws
    }

    void
    use(FILE *)
    {
    }
} // namespace ex10

namespace ex11
{
    struct X
    {
    };

    void
    test()
    {
        std::shared_ptr<X> pa, pb, pc;

        pa = std::make_shared<X>();
        // use-count always starts at 1

        pb = pa;
        // make a copy of the pointer; use-count is now 2

        pc = std::move(pa);
        assert(pa == nullptr);
        // moving the pointer keeps the use-count at 2

        pb = nullptr;
        // decrement the use-count back to 1
        assert(pc.use_count() == 1);
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
        auto p = std::make_shared<Super>(4, 2);
        return std::shared_ptr<int>(p, &p->second);
    }

    void
    test()
    {
        std::shared_ptr<int> q = get_second();
        puts("accessing Super::second");
        assert(*q == 2);
    }
} // namespace ex12

namespace ex13
{
    struct X
    {
    };
    void
    test()
    {
        std::shared_ptr<X> pa, pb, pc;

        pa = std::make_shared<X>();
        // use-count always starts at 1

        pb = pa;
        // make a copy of the pointer; use-count is now 2

        pc = std::shared_ptr<X>(pb.get()); // WRONG!
                                           // give the same pointer to shared_ptr again,
                                           // which tells shared_ptr to manage it -- twice!
        assert(pb.use_count() == 2);
        assert(pc.use_count() == 1);

        pc = nullptr;
        // pc's use-count drops to zero and shared_ptr
        // calls "delete" on the X object

        *pb; // accessing the freed object yields undefined behavior
    }
} // namespace ex13

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
        }
    };
} // namespace ex15

namespace ex16
{
    struct CorrectWatcher
    {
        std::weak_ptr<int> m_ptr;

        void
        watch(const std::shared_ptr<int> &p)
        {
            m_ptr = std::weak_ptr<int>(p);
        }

        int
        current_value() const
        {
            // Now we can safely ask whether *m_ptr has been
            // deallocated or not.
            if (auto p = m_ptr.lock())
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

namespace ex17
{
    using std::shared_ptr;
    using std::weak_ptr;

    template <class T>
    class enable_shared_from_this
    {
        weak_ptr<T> m_weak;

      public:
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
    puts(const char *)
    {
        caught += 1;
    }

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
        auto sa = std::make_shared<Widget>();

        assert(sa.use_count() == 1);
        sa->call_on_me([](auto sb) { assert(sb.use_count() == 2); });

        Widget w;
        try
        {
            w.call_on_me([](auto) {});
        }
        catch (const std::bad_weak_ptr &)
        {
            puts("Caught!");
        }
    }

    void
    test2()
    {
        caught = 0;
        test1();
        assert(caught == 1);
    }
} // namespace ex18

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
        assert(copies == 1 && moves == 1);

        copies = moves = 0;
        D1 r3          = a + std::move(b);
        assert(copies == 1 && moves == 1);

        copies = moves = 0;
        D1 r4          = std::move(a) + std::move(b);
        assert(copies == 1 && moves == 1);

        // unused
        (void)r1;
        (void)r2;
        (void)r3;
        (void)r4;
    }
} // namespace ex19

namespace ex20
{
    template <class Derived>
    class addable
    {
      public:
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
        D1 r{a + b};
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
        (void)r;
        (void)r2;
        (void)r3;
        (void)r4;
    }
} // namespace ex20

namespace ex21
{
    class Widget;
    void remusnoc(std::unique_ptr<Widget> p);

    std::unique_ptr<Widget> recudorp();
} // namespace ex21

namespace ex22
{
    class Widget;
    void suougibma(Widget *p);
} // namespace ex22

namespace ex23
{
    class Widget;

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

    void revresbo(observer_ptr<Widget> p);
} // namespace ex23

int
main()
{
    ex01::test1();
    ex01::test2();
    ex01::test3();
    ex10::test();
    ex11::test();
    ex12::test();
    // ex13::test(); // has undefined behavior; crashes
    ex18::test1();
    // ex19::test(); // crashes
    ex20::test();
}
