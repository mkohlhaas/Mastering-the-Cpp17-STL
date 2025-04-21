#include <algorithm>
#include <any>
#include <boost/variant.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

// Vocabulary Types

// The story of std::string

namespace ex01
{
    char *
    greet(const char *name)
    {
        char buffer[100];
        snprintf(buffer, 100, "hello %s", name);
        return strdup(buffer);
    }

    void
    test()
    {
        const char *who = "world";
        char       *hw  = greet(who);
        assert(strcmp(hw, "hello world") == 0);
        free(hw);
    }
} // namespace ex01

namespace ex02
{
    std::string
    greet(const std::string &name)
    {
        return "hello " + name;
    }

    void
    test()
    {
        std::string who = "world";
        assert(greet(who) == "hello world");
    }
} // namespace ex02

// Tagging reference types with reference_wrapper

namespace ex12
{
    namespace std
    {
        template <typename T>
        class reference_wrapper
        {
          private:
            T *m_ptr;

          public:
            reference_wrapper(T &t) noexcept : m_ptr(&t)
            {
            }

            operator T &() const noexcept
            {
                return *m_ptr;
            }

            T &
            get() const noexcept
            {
                return *m_ptr;
            }
        };

        template <typename T>
        reference_wrapper<T> ref(T &t);
    } // namespace std
} // namespace ex12

namespace ex13
{
    // The constructor of std::thread is written with specific special cases to handle
    // reference_wrapper parameters by "decaying" them into native references. The same
    // special cases apply to the standard library functions make_pair, make_tuple, bind,
    // invoke, and everything based on invoke (such as std::apply, std::function::operator(),
    // and std::async).

    void
    test()
    {
        int  result = 0;
        auto task   = [](int &r) { r = 42; };

        // Trying to use a native reference wouldn't compile.
        // std::thread t(task, result);

        // Correctly pass result "by reference" to the new thread.
        std::thread t(task, std::ref(result));

        t.join();
        assert(result == 42);
    }
} // namespace ex13

// C++11 and algebraic types

namespace ex03
{
    enum class Color
    {
        RED   = 1,
        BLACK = 2,
    };

    enum class Size
    {
        SMALL  = 1,
        MEDIUM = 2,
        LARGE  = 3,
    };

    using sixtype  = std::pair<Color, Size>;    // product type
    using fivetype = std::variant<Color, Size>; // sum type
} // namespace ex03

namespace ex06
{
    template <class A, class B>
    struct pair
    {
        A first;
        B second;
    };
} // namespace ex06

// Working with std::tuple

namespace ex08
{
    void
    test()
    {
        using Author = std::pair<std::string, std::string>;

        std::vector<Author> authors = {
            {"Fyodor", "Dostoevsky"},
            {"Sylvia", "Plath"},
            {"Vladimir", "Nabokov"},
            {"Douglas", "Hofstadter"},
        };

        // Sort by first name then last name.
        // std::tie creates a tuple of lvalue references to its arguments or instances of std::ignore.
        std::sort(authors.begin(), authors.end(), [](auto &&autor_a, auto &&author_b) {
            return std::tie(autor_a.first, autor_a.second) < std::tie(author_b.first, author_b.second);
        });

        assert(authors[0] == Author("Douglas", "Hofstadter"));
        assert((authors == std::vector<Author>{
                               {"Douglas", "Hofstadter"},
                               {"Fyodor", "Dostoevsky"},
                               {"Sylvia", "Plath"},
                               {"Vladimir", "Nabokov"},
                           }));

        // Sort by last name then first name.
        std::sort(authors.begin(), authors.end(), [](auto &&author_a, auto &&author_b) {
            return std::tie(author_a.second, author_a.first) < std::tie(author_b.second, author_b.first);
        });
        assert(authors[0] == Author("Fyodor", "Dostoevsky"));
        assert((authors == std::vector<Author>{
                               {"Fyodor", "Dostoevsky"},
                               {"Douglas", "Hofstadter"},
                               {"Vladimir", "Nabokov"},
                               {"Sylvia", "Plath"},
                           }));
    }
} // namespace ex08

namespace ex07
{
    // simulating the "multiple assignment" found in languages such as Python

    void
    test()
    {
        using std::string;

        // assign both s and i at once
        string s;
        int    i;
        std::tie(s, i) = std::make_tuple("hello", 42);
    }
} // namespace ex07

namespace ex10
{
    // Lastly, in C++17 we are allowed to use constructor template parameter deduction to write
    // simply std::tuple(a, b, c...); but it's probably best to avoid this feature unless you
    // know specifically that you want its behaviour.

    void
    test()
    {
        auto [i, j, k] = std::tuple{1, 2, 3};

        // make_tuple decays reference_wrapper...
        auto t1 = std::make_tuple(i, std::ref(j), k); // tuple<int, int &, int>
        static_assert(std::is_same_v<decltype(t1), std::tuple<int, int &, int>>);

        // ...whereas the deduced constructor does not.
        auto t2 = std::tuple(i, std::ref(j), k); // tuple<int, std::reference_wrapper<int>, int>
        static_assert(std::is_same_v<decltype(t2), std::tuple<int, std::reference_wrapper<int>, int>>);
    }
} // namespace ex10

// Manipulating tuple values

namespace ex09
{
    // The standard provides std::tuple_size_v<decltype(t)>.
    // These implementations might be easier to use.

    template <class T>
    constexpr size_t
    tuple_size_1(T &&)
    {
        return std::tuple_size_v<std::remove_reference_t<T>>;
    }

    template <class... Ts>
    constexpr size_t
    tuple_size_2(const std::tuple<Ts...> &)
    {
        return sizeof...(Ts); // much easier
    }

    void
    test()
    {
        std::tuple<int, double> t;

        static_assert(tuple_size_1(t) == 2);
        static_assert(tuple_size_2(t) == 2);

        static_assert(tuple_size_1(std::make_tuple(1, 2, 3)) == 3);
        static_assert(tuple_size_2(std::make_tuple(1, 2, 3)) == 3);
    }
} // namespace ex09

namespace ex11
{
    // std::forward_as_tuple will accept any kind of references as input, and
    // perfectly forward them into the tuple so that they can later be extracted by
    // std::get<I>(t)...:

    template <typename F>
    void run_zeroarg(const F &f);

    template <typename F, typename... Args>
    void
    run_multiarg(const F &f, Args &&...args)
    {
        auto fwd_args = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda   = [&f, fwd_args]() { std::apply(f, fwd_args); };
        run_zeroarg(f);
    }
} // namespace ex11

// A note about named classes
//
// As we saw in Chapter 4, The Container Zoo, when we compared std::array<double, 3>
// to struct Vec3, using an STL class template can shorten your development time and
// eliminate sources of error by reusing well-tested STL components; but it can also make your
// code less readable or give your types too much functionality. In our example from Chapter 4,
// The Container Zoo, std::array<double, 3> turned out to be a poor choice for Vec3
// because it exposed an unwanted operator<.
//
// Using any of the algebraic types (tuple, pair, optional, or variant) directly in your
// interfaces and APIs is probably a mistake. You'll find that your code is easier to read,
// understand, and maintain if you write named classes for your own "domain-specific
// vocabulary" types, even if - especially if - they end up being thin wrappers around the
// algebraic types.

// Expressing alternatives with std::variant

namespace ex14
{
    // std::variant represents a type-safe union.

    void
    test()
    {
        std::variant<int, double> v1;

        v1 = 1;    // activate the "int" member
        assert(v1.index() == 0);
        assert(std::get<0>(v1) == 1);

        v1 = 3.14; // activate the "double" member
        assert(v1.index() == 1);
        assert(std::get<1>(v1) == 3.14);
        assert(std::get<double>(v1) == 3.14);

        // std::holds_alternative checks if the variant v holds the alternative T.
        assert(std::holds_alternative<int>(v1) == false);
        assert(std::holds_alternative<double>(v1) == true);

        // get_if takes a pointer
        assert(std::get_if<int>(&v1) == nullptr);  // ... and returns nullptr on error
        assert(*std::get_if<double>(&v1) == 3.14); // ... and returns pointer to stored value

        //  worst...
        try
        {
            std::cout << std::get<int>(v1) << std::endl;
        }
        catch (const std::bad_variant_access &)
        {
            std::cout << "line " << __LINE__ << ": bad_variant_access" << std::endl;
        }

        //  worst...
        try
        {
            // you can use index or type
            std::cout << std::get<0>(v1) << std::endl;
        }
        catch (const std::bad_variant_access &)
        {
            std::cout << "line " << __LINE__ << ": bad_variant_access" << std::endl;
        }

        v1 = 314;

        // still bad...
        if (v1.index() == 0)
        {
            std::cout << "line " << __LINE__ << ": " << std::get<int>(v1) << std::endl; // 314
        }

        // slightly better...
        if (std::holds_alternative<int>(v1))
        {
            std::cout << "line " << __LINE__ << ": " << std::get<int>(v1) << std::endl; // 314
        }

        // ...best.
        if (int *p = std::get_if<int>(&v1))
        {
            std::cout << "line " << __LINE__ << ": " << *p << std::endl; // 314
        }
    }
} // namespace ex14

// Visiting variants

namespace ex16
{
    using Var = std::variant<int, double, std::string>;

    struct Visitor
    {
        // from index 0 of variant Var
        double
        operator()(int i)
        {
            return double(i);
        }

        // from index 1 of variant Var
        double
        operator()(double d)
        {
            return d;
        }

        // from index 2 of variant Var
        double
        operator()(const std::string &)
        {
            return -1;
        }
    };

    void
    show(Var v)
    {
        std::cout << "line " << __LINE__ << ": " << std::visit(Visitor{}, v) << std::endl;
    }

    void
    test()
    {
        show(3.14);          // 3.14
        show(2);             // 2
        show("hello world"); // -1
    }
} // namespace ex16

namespace ex17
{
    // we can wrap this up into a template function or - the most common case - a C++14 generic lambda

    using Var = std::variant<int, double, std::string>;

    void
    show(Var v)
    {
        std::visit(
            // generic lambda
            [](const auto &alt) {
                if constexpr (std::is_same_v<decltype(alt), const std::string &>)
                {
                    std::cout << "line " << __LINE__ << ": " << double(-1) << std::endl;
                }
                else
                {
                    std::cout << "line " << __LINE__ << ": " << double(alt) << std::endl;
                }
            },
            v);
    }

    void
    test()
    {
        show(3.14);          // 3.14
        show(2);             // 2
        show("hello world"); // -1
    }
} // namespace ex17

namespace ex18
{
    // std::visit is actually variadic!

    struct MultiVisitor
    {
        template <class T, class U, class V>
        void
        operator()(T, U, V) const
        {
            puts("wrong");
        }

        void
        operator()(char, int, double) const
        {
            puts("right!");
        }
    };

    void
    test()
    {
        std::variant<double, char, int> v1 = 3.14;
        std::variant<char, int, double> v2 = 2;
        std::variant<int, double, char> v3 = 'x';

        std::visit(MultiVisitor{}, v1, v2, v3); // "wrong"
        std::visit(MultiVisitor{}, v2, v1, v3); // "wrong"
        std::visit(MultiVisitor{}, v3, v2, v1); // "right!"
    }
} // namespace ex18

// What about make_variant? and a note on value semantics.
// There is no make_variant!

namespace ex19
{
    // valueless state is not possible any more

    struct A
    {
        A()
        {
            throw "ha ha!";
        }
    };

    struct B
    {
        operator int()
        {
            throw "ha ha!";
        }
    };

    struct C
    {
        C()                = default;
        C &operator=(C &&) = default;

        C(C &&)
        {
            throw "ha ha!";
        }
    };

    void
    test()
    {
        std::variant<int, A, C> v1 = 42;

        try
        {
            v1.emplace<A>();
        }
        catch (const char *haha)
        {
            std::cout << "line " << __LINE__ << ": " << haha << std::endl; // haha
        }
        // valueless_by_exception returns false if and only if the variant holds a value
        assert(!v1.valueless_by_exception());

        try
        {
            v1.emplace<int>(B());
        }
        catch (const char *haha)
        {
            std::cout << "line " << __LINE__ << ": " << haha << std::endl; // haha
        }
        assert(!v1.valueless_by_exception());
    }
} // namespace ex19

namespace ex20
{
    // valueless state can still happen in this scenario

    struct A
    {
        A()
        {
            throw "ha ha!";
        }
    };

    struct B
    {
        operator int()
        {
            throw "ha ha!";
        }
    };

    struct C
    {
        C()                = default;
        C &operator=(C &&) = default;

        // Your move constructors should always be marked noexcept!!!
        // If you mark your move constructor as noexcept a valueless state cannot happen.
        C(C &&)
        {
            throw "ha ha!"; // Move constructor should never throw!!!
        }
    };

    void
    test()
    {
        std::variant<int, A, C> v1 = 42;

        // this is the only difference to namespace19 (using assignment operator)
        v1 = 42;

        // Constructing the right-hand side of this assignment
        // will throw; yet the variant is unaffected.
        try
        {
            v1 = A();
        }
        catch (const char *haha)
        {
            std::cout << "line " << __LINE__ << ": " << haha << std::endl; // haha
        }
        assert(std::get<int>(v1) == 42);

        // In this case as well.
        try
        {
            v1 = B();
        }
        catch (const char *haha)
        {
            std::cout << "line " << __LINE__ << ": " << haha << std::endl; // haha
        }
        assert(std::get<int>(v1) == 42);

        // But a throwing move-constructor can still foul it up.
        try
        {
            v1 = C();
        }
        catch (const char *haha)
        {
            std::cout << "line " << __LINE__ << ": " << haha << std::endl; // haha
        }
        // now we have a valueless state
        assert(v1.valueless_by_exception());
    }
} // namespace ex20

// Delaying initialization with std::optional

namespace ex21
{
    // faking optional with variant

    static void
    use(int)
    {
    }

    static int some_default = 0;

    using map_resource_limit = std::map<std::string, int>;

    map_resource_limit g_limits = {
        {"memory", 655360},
    };

    // std::monostate is a UNIT TYPE intended for use
    // as a well-behaved empty alternative in std::variant.

    std::variant<std::monostate, int>
    get_resource_limit(const std::string &key)
    {
        if (auto it = g_limits.find(key); it != g_limits.end())
        {
            return it->second;
        }
        return std::monostate{};
    }

    void
    test()
    {
        auto limit = get_resource_limit("memory");
        if (std::holds_alternative<int>(limit))
        {
            use(std::get<int>(limit));
        }
        else
        {
            use(some_default);
        }
    }
} // namespace ex21

// You can remember this behavior by noticing that the C++ standard library
// LIKES TO USE PUNCTUATION FOR ITS MOST EFFICIENT, LEAST SANITY-CHECKED
// OPERATIONS. For example, std::vector::operator[] does less bounds-checking
// than std::vector::at(). Therefore, by the same logic, std::optional::operator*
// does less bounds-checking than std::optional::value().

namespace ex22
{
    static void
    use(int)
    {
    }

    static int some_default = 0;

    using map_resource_limit = std::map<std::string, int>;

    map_resource_limit g_limits = {
        {"memory", 655360},
    };

    std::optional<int>
    get_resource_limit(const std::string &key)
    {
        if (auto it = g_limits.find(key); it != g_limits.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void
    test()
    {
        auto limit = get_resource_limit("memory");

        // preferred syntax
        if (limit.has_value()) // if (limit)
        {
            use(*limit);
        }
        else
        {
            use(some_default);
        }
    }
} // namespace ex22

namespace ex23
{
    static void
    use(int i)
    {
        assert(i == 42);
    }

    static int some_default = 42;

    std::optional<int>
    get_resource_limit(const std::string &)
    {
        return std::nullopt;
    }

    void
    test()
    {
        auto limit = get_resource_limit("memory");
        use(limit.value_or(some_default));
    }
} // namespace ex23

namespace ex24
{
    // Another common and useful way to use std::optional<T> is as a way to
    // handle "not yet a T" at rest, as a class data member.

    auto
    make_lambda(int arg)
    {
        return [arg](int x) { return x + arg; };
    }

    using L = decltype(make_lambda(0));

    // L is not default constructable, not move assignable
    static_assert(!std::is_default_constructible_v<L>);
    static_assert(!std::is_move_assignable_v<L>);

    class ProblematicAdder
    {
        L fn_;
    };

    // So a class with an L member is also not default constructable.
    static_assert(!std::is_default_constructible_v<ProblematicAdder>);

    class Adder
    {
      private:
        // wrapping L up in std::optional makes class default constructable
        std::optional<L> fn_;

      public:
        void
        setup(int first_arg)
        {
            fn_.emplace(make_lambda(first_arg)); // put lambda into optional
        }

        int
        call(int second_arg)
        {
            // this will throw unless setup() was called first
            return fn_.value()(second_arg);
        }
    };

    // Adder is default constructable although L isn't.
    static_assert(std::is_default_constructible_v<Adder>);

    void
    test()
    {
        // error: call to implicitly-deleted default constructor
        // ProblematicAdder problematicAdder;

        Adder adder;                // default constructed
        adder.setup(4);             // First setup ...
        int result = adder.call(5); // ... then call.
        assert(result == 9);
    }
} // namespace ex24

// Revisiting variant

namespace ex27
{
#if 0
// recursive data structure not possible
using JSONValue = std::variant<
    std::nullptr_t,
    bool,
    double,
    std::string,
    std::vector<JSONValue>,
    std::map<std::string, JSONValue>
>;
#endif
} // namespace ex27

namespace ex28
{
    // workaround by using boost::variant
    using JSONValue = boost::variant<std::nullptr_t,
                                     bool,                                              //
                                     double,                                            //
                                     std::string,                                       //
                                     std::vector<boost::recursive_variant_>,            //
                                     std::map<std::string, boost::recursive_variant_>>; //
} // namespace ex28

namespace ex29
{
    // Workaround by introducing a new class type called JSONValue
    // as forward references to class types are acceptable to C++.
    struct JSONValue
    {
        std::variant<std::nullptr_t,
                     bool,                             //
                     double,                           //
                     std::string,                      //
                     std::vector<JSONValue>,           //
                     std::map<std::string, JSONValue>> //
            value_;
    };
} // namespace ex29

// Infinite alternatives with std::any

// std::any describes a type-safe container for single values of any copy constructible type.

namespace ex30
{
    void
    use(std::string &)
    {
        std::cout << "line " << __LINE__ << ": don't go fish" << std::endl;
    }

    void
    test()
    {
        std::any a; // construct an empty container

        assert(!a.has_value());

        a = 42;
        assert(a.has_value());
        assert(a.type() == typeid(int));

        a = std::string("hello");
        assert(a.has_value());
        assert(a.type() == typeid(std::string));

        // This version of any_cast doesn't throw! (passing a pointer - &a)
        if (std::string *p = std::any_cast<std::string>(&a))
        {
            use(*p);
        }
        else
        {
            std::cout << "line " << __LINE__ << ": go fish" << std::endl;
        }

        // This version of any_cast doesn throw! (passing a value)
        // See documentation for std::any_cast and its overloads.
        try
        {
            std::string &s = std::any_cast<std::string &>(a);
            use(s);
        }
        catch (const std::bad_any_cast &)
        {
            std::cout << "line " << __LINE__ << ": go fish" << std::endl;
        }
    }
} // namespace ex30

namespace ex32
{
    template <class T>
    struct Widget
    {
    };

    std::any
    get_widget()
    {
        return std::make_any<Widget<int>>();
    }
} // namespace ex32

namespace ex33
{
    template <class F>
    int
    hypothetical_any_visit(F &&, std::any)
    {
        return 1;
    }

    template <class T>
    struct Widget
    {
    };

    std::any
    get_widget()
    {
        return std::make_any<Widget<int>>();
    }

    template <class T>
    int
    size(Widget<T> &w)
    {
        return sizeof w;
    }

    void
    test()
    {
        std::any a  = get_widget();
        int      sz = hypothetical_any_visit([](auto &&w) { return size(w); }, a);
        assert(sz == sizeof(Widget<int>));
    }
} // namespace ex33

// std::any versus polymorphic class types

namespace ex34
{
    // std::any is one hundred percent statically type-safe: there is no way to
    // break into it and get a "pointer to the data" (for example, a void *)
    // without knowing the exact static type of that data.

    struct Animal
    {
        virtual ~Animal() = default;
    };

    struct Cat : Animal
    {
    };

    void
    test()
    {
        std::any a = Cat{};

        // The held object is a "Cat"...
        assert(a.type() == typeid(Cat));
        assert(std::any_cast<Cat>(&a) != nullptr);

        // Asking for a base "Animal" will not work.
        assert(a.type() != typeid(Animal));
        assert(std::any_cast<Animal>(&a) == nullptr);

        // Asking for void* certainly will not work!
        assert(std::any_cast<void *>(&a) == nullptr);
    }
} // namespace ex34

// Type erasure in a nutshell

namespace ex35
{
    template <typename T>
    struct AnyImpl;

    class any;
    struct AnyBase
    {
        virtual const std::type_info &type()         = 0;
        virtual void                  copy_to(any &) = 0;
        virtual void                  move_to(any &) = 0;

        virtual ~AnyBase() = default;
    };

    class any
    {
      private:
        std::unique_ptr<AnyBase> p_ = nullptr;

      public:
        template <typename T, typename... Args>
        std::decay_t<T> &emplace(Args &&...args);

        void reset() noexcept;

        const std::type_info &type() const;

        any(const any &rhs);

        any &operator=(const any &rhs);
    };

    template <typename T>
    struct AnyImpl : AnyBase
    {
        T t_;
        const std::type_info &
        type()
        {
            return typeid(T);
        }

        void
        copy_to(any &rhs) override
        {
            rhs.emplace<T>(t_);
        }

        void
        move_to(any &rhs) override
        {
            rhs.emplace<T>(std::move(t_));
        }

        // the destructor doesn't need anything
        // special in this case
    };
} // namespace ex35

namespace ex36
{
    class any;

    struct AnyBase
    {
        virtual const std::type_info &type()         = 0;
        virtual void                  copy_to(any &) = 0;
        virtual void                  move_to(any &) = 0;
        virtual ~AnyBase()                           = default;
    };

    template <typename T>
    struct AnyImpl : AnyBase
    {
        T                     t_;
        const std::type_info &type();
        void                  copy_to(any &rhs) override;
        void                  move_to(any &rhs) override;
    };

    // ex36
    class any
    {
        std::unique_ptr<AnyBase> p_ = nullptr;

      public:
        template <typename T, typename... Args>
        std::decay_t<T> &
        emplace(Args &&...args)
        {
            p_ = std::make_unique<AnyImpl<T>>(std::forward<Args>(args)...);
        }

        bool
        has_value() const noexcept
        {
            return (p_ != nullptr);
        }

        void
        reset() noexcept
        {
            p_ = nullptr;
        }

        const std::type_info &
        type() const
        {
            return p_ ? p_->type() : typeid(void);
        }

        any(const any &rhs)
        {
            *this = rhs;
        }

        any &
        operator=(const any &rhs)
        {
            if (rhs.has_value())
            {
                rhs.p_->copy_to(*this);
            }
            return *this;
        }
    };
} // namespace ex36

namespace ex37
{
    using Ptr = std::unique_ptr<int>;

    template <class T>
    struct Shim
    {
        T
        get()
        {
            return std::move(*t_);
        }

        template <class... Args>
        Shim(Args &&...args) : t_(std::in_place, std::forward<Args>(args)...)
        {
        }

        Shim(Shim &&)            = default;
        Shim &operator=(Shim &&) = default;

        Shim(const Shim &)
        {
            throw "oops";
        }

        Shim &
        operator=(const Shim &)
        {
            throw "oops";
        }

      private:
        std::optional<T> t_;
    };

    void
    test()
    {
        Ptr p = std::make_unique<int>(42);

        // Ptr cannot be stored in std::any because it is move-only.
        // std::any a = std::move(p);

        // But Shim<Ptr> can be!
        std::any a = Shim<Ptr>(std::move(p));
        assert(a.type() == typeid(Shim<Ptr>));

        // Moving a Shim<Ptr> is okay...
        std::any b = std::move(a);

        try
        {
            // ...but copying a Shim<Ptr> will throw.
            std::any c = b;
        }
        catch (...)
        {
        }

        // Get the move-only Ptr back out of the Shim<Ptr>.
        Ptr r = std::any_cast<Shim<Ptr> &>(b).get();
        assert(*r == 42);
    }
} // namespace ex37

namespace ex38
{
    int
    my_abs(int x)
    {
        return x < 0 ? -x : x;
    }

    long
    unusual(long x, int y = 3)
    {
        return x + y;
    }

    void
    test()
    {
        std::function<int(int)> f;             // construct an empty container
        assert(!f);

        f = my_abs;                            // store a function in the container
        assert(f(-42) == 42);

        f = [](long x) { return unusual(x); }; // or a lambda!
        assert(f(-42) == -39);
    }
} // namespace ex38

namespace ex39
{
    void
    test()
    {
        std::function<int(int)> f; // construct an empty container

        f = [i = 0](int) mutable { return ++i; };
        assert(f(-42) == 1);
        assert(f(-42) == 2);

        auto g = f;
        assert(f(-42) == 3);
        assert(f(-42) == 4);
        assert(g(-42) == 3);
        assert(g(-42) == 4);
    }
} // namespace ex39

namespace ex40
{
    void
    use(int (*)(int))
    {
    }

    void
    test()
    {
        std::function<int(int)> f;

        if (f.target_type() == typeid(int (*)(int)))
        {
            int (*p)(int) = *f.target<int (*)(int)>();
            use(p);
        }
        else
        {
            // go fish!
        }
    }
} // namespace ex40

namespace ex41
{
    void
    test()
    {

        auto capture = [](auto &p) {
            using T = std::decay_t<decltype(p)>;
            return std::make_shared<T>(std::move(p));
        };

        std::promise<int> p;

        std::function<void()> f = [sp = capture(p)]() { sp->set_value(42); };
    }
} // namespace ex41

namespace ex42
{
    // templated_for_each is a template and must be visible at the
    // point where it is called.
    template <class F>
    void
    templated_for_each(std::vector<int> &v, F f)
    {
        for (int &i : v)
        {
            f(i);
        }
    }

    // type_erased_for_each has a stable ABI and a fixed address.
    // It can be called with only its declaration in scope.
    extern void type_erased_for_each(std::vector<int> &, std::function<void(int)>);
} // namespace ex42

int
main()
{
    ex01::test();
    ex02::test();
    ex07::test();
    ex08::test();
    ex09::test();
    ex10::test();
    ex13::test();
    ex14::test();
    ex16::test();
    ex17::test();
    ex18::test();
    ex19::test();
    ex20::test();
    ex21::test();
    ex22::test();
    ex23::test();
    ex24::test();
    ex30::test();
    ex33::test();
    ex34::test();
    ex37::test();
    ex38::test();
    ex39::test();
    ex40::test();
    ex41::test();
}
