#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <forward_list>
#include <functional>
#include <iostream>
#include <list>
#include <numeric>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

// classify each standard algorithm as:
// - read-only
// - write-only
// - transformative
// - permutative
// - one-range
// - two-range
// - one-and-a-half range

// Read-only range algorithms

namespace ex01
{
    void
    test()
    {
        constexpr int N = 10;

        int a[N];

        // A correct for-loop.
        for (int i = 0; i < N; ++i)
        {
            // ...
        }

        // one variety of "smelly" for-loop
        for (int i = 0; i <= N; ++i)
        {
            // ...
        }

        // a correct invocation of a standard algorithm
        [[maybe_unused]] auto c1 = std::count_if(std::begin(a), std::end(a), [](int) { return true; });

        // A "smelly" invocation
        [[maybe_unused]] auto c2 = std::count_if(std::begin(a), std::end(a) - 1, [](int) { return true; });

        // a "trivial" invocation: counting a range of length zero
        // empty range -> compiler already knows the answer: 0
        [[maybe_unused]] auto c3 = std::count_if(std::begin(a), std::begin(a), [](int) { return true; });
    }
} // namespace ex01

namespace ex02
{
    void
    test()
    {
        int                    a[]{1, 2, 3, 4, 5};
        std::list<int>         lst{1, 2, 3, 4, 5};
        std::forward_list<int> flst{1, 2, 3, 4, 5};

        assert(std::distance(std::begin(a), std::end(a)) == 5);
        assert(std::distance(std::begin(lst), std::end(lst)) == 5);
        assert(std::distance(std::begin(flst), std::end(flst)) == 5);

        // vector is a random access iterator
        // gives negative value
        assert(std::distance(std::end(a), std::begin(a)) == -5);

        // std::list is a bidirectional iterator
        // The following line gives an "incorrect" answer!
        assert(std::distance(std::end(lst), std::begin(lst)) == 1);

        // std::forward_list is a forward iterator
        // And this one just segfaults!
        // [[maybe_unused]] auto i = std::distance(std::end(flst), std::begin(flst));
    }
} // namespace ex02

namespace ex03
{
    void
    test()
    {
        std::set<int> s{1, 2, 3, 10, 42, 99};

        // O(n): compare each element with 42
        // has to loop over all elements (no insight into underlying data structure)
        [[maybe_unused]] auto present1 = std::count(s.begin(), s.end(), 42);

        // O(log n): ask the container to look up 42 itself
        [[maybe_unused]] auto present2 = s.count(42);
    }
} // namespace ex03

namespace ex04
{
    template <typename InputIterator, typename UnaryPredicate>
    InputIterator
    find_if(InputIterator first, InputIterator last, UnaryPredicate p)
    {
        for (; first != last; ++first)
        {
            if (p(*first))
            {
                return first; // short-circuiting (not going through the whole range)
            }
        }
        return last;
    }

    template <typename It, typename U>
    It
    find_if_not(It first, It last, U p)
    {
        return std::find_if(first, last, [&](auto &&e) { return not p(e); });
    }

    template <typename It, typename T>
    It
    find(It first, It last, T value)
    {
        return std::find_if(first, last, [&](auto &&e) { return e == value; });
    }

    template <typename It, typename UnaryPredicate>
    bool
    all_of(It first, It last, UnaryPredicate p)
    {
        return std::find_if_not(first, last, p) == last;
    }

    template <typename It, typename U>
    bool
    any_of(It first, It last, U p)
    {
        return std::find_if(first, last, p) != last;
    }

    template <typename It, typename U>
    bool
    none_of(It first, It last, U p)
    {
        return std::find_if(first, last, p) == last;
    }
} // namespace ex04

namespace ex06
{
    template <typename It, typename FwdIt>
    It
    find_first_of(It first, It last, FwdIt targetfirst, FwdIt targetlast)
    {
        return std::find_if(first, last, [&](auto &&e) {
            return std::any_of(targetfirst, targetlast, [&](auto &&t) { return e == t; });
        });
    }

    template <typename It, typename FwdIt, typename BinaryPredicate>
    It
    find_first_of(It first, It last, FwdIt targetfirst, FwdIt targetlast, BinaryPredicate p)
    {
        return std::find_if(first, last, [&](auto &&e) {
            return std::any_of(targetfirst, targetlast, [&](auto &&t) { return p(e, t); });
        });
    }

    void
    test()
    {
        std::vector<int> v{1, 2, 3, 4};
        std::vector<int> t{5, 3};

        auto it1 = ex06::find_first_of(v.begin(), v.end(), t.begin(), t.end());
        auto it2 = std::find_first_of(v.begin(), v.end(), t.begin(), t.end());

        assert(it1 == it2);
        assert(*it1 == 3);
    }
} // namespace ex06

namespace ex07
{
    void
    test()
    {
        std::istream_iterator<char> ii(std::cin);
        std::istream_iterator<char> iend{};

        std::string s = "hello";

        // Chomp characters from std::cin until finding an 'h', 'e', 'l', or 'o'.
        auto _ = std::find_first_of(ii, iend, s.begin(), s.end());
    }
} // namespace ex07

namespace ex08
{
    // mismatch

    template <typename It1, typename It2, typename B>
    auto
    mismatch(It1 first1, It1 last1, It2 first2, It2 last2, B p)
    {
        while (first1 != last1 && first2 != last2 && p(*first1, *first2))
        {
            ++first1;
            ++first2;
        }
        return std::make_pair(first1, first2);
    }

    template <typename It1, typename It2>
    auto
    mismatch(It1 first1, It1 last1, It2 first2, It2 last2)
    {
        return std::mismatch(first1, last1, first2, last2, std::equal_to<>{});
    }

    // equal

    template <typename T>
    constexpr bool is_random_access_iterator_v =
        std::is_base_of_v<std::random_access_iterator_tag, typename std::iterator_traits<T>::iterator_category>;

    template <typename It1, typename It2, typename B>
    bool
    equal(It1 first1, It1 last1, It2 first2, It2 last2, B p)
    {
        if constexpr (is_random_access_iterator_v<It1> && is_random_access_iterator_v<It2>)
        {
            // Ranges of different lengths can never be equal.
            if ((last2 - first2) != (last1 - first1))
            {
                return false;
            }
        }
        return std::mismatch(first1, last1, first2, last2, p) == std::make_pair(last1, last2);
    }

    template <typename It1, typename It2>
    bool
    equal(It1 first1, It1 last1, It2 first2, It2 last2)
    {
        return std::equal(first1, last1, first2, last2, std::equal_to<>{});
    }
} // namespace ex08

// Shunting data with std::copy

namespace ex09
{
    // copy is a one-and-a-half-range algorithm (end iterator of second range is not given)
    template <typename InIt, typename OutIt>
    OutIt
    copy(InIt first1, InIt last1, OutIt destination)
    {
        while (first1 != last1)
        {
            *destination = *first1;
            ++first1;
            ++destination;
        }
        return destination;
    }

    // copy can also be used for feeding data to an arbitrary "sink" function

    class putc_iterator : public boost::iterator_facade<putc_iterator,       // T
                                                        const putc_iterator, // value_type
                                                        std::output_iterator_tag>
    {
      private:
        friend class boost::iterator_core_access;

        auto &
        dereference() const
        {
            return *this; // this iterator is its own proxy object!
        }

        // increment doesn't do anything (noop)
        void
        increment()
        {
        }

        bool
        equal(const putc_iterator &) const
        {
            return false;
        }

      public:
        void
        operator=(char ch) const
        {
            putc(ch, stdout); // this iterator is its own proxy object!
        }
    };

    void
    test()
    {
        std::string s = "hello\n";
        std::copy(s.begin(), s.end(), putc_iterator{});
    }
} // namespace ex09

namespace ex11
{
    namespace std
    {
        using ::std::copy;
        using ::std::move;
        using ::std::output_iterator_tag;
        using ::std::string;
        using ::std::vector;
    } // namespace std

    namespace std
    {
        template <typename Container>
        class back_insert_iterator
        {
          private:
            using ContainerValueType = typename Container::value_type;

            Container *c;

          public:
            // back_insert_iterator is an output_iterator
            using iterator_category = output_iterator_tag;
            using difference_type   = void;
            using value_type        = void;
            using pointer           = void;
            using reference         = void;

            explicit back_insert_iterator(Container &ctr) : c(&ctr)
            {
            }

            auto &
            operator*()
            {
                return *this;
            }

            auto &
            operator++()
            {
                return *this;
            }

            auto &
            operator++(int)
            {
                return *this;
            }

            auto &
            operator=(const ContainerValueType &v)
            {
                c->push_back(v);
                return *this;
            }

            auto &
            operator=(ContainerValueType &&v)
            {
                c->push_back(std::move(v));
                return *this;
            }
        };

        template <typename Container>
        auto
        back_inserter(Container &c)
        {
            return back_insert_iterator<Container>(c);
        }
    } // namespace std

    void
    test()
    {
        std::string s = "hello";

        std::vector<char> dest;
        std::copy(s.begin(), s.end(), std::back_inserter(dest));

        // Also possible because of CTAD (Class Template Argument Deduction).
        // But cumbersome - too many characters - not recommended!
        std::copy(s.begin(), s.end(), std::back_insert_iterator(dest));
        assert(dest.size() == 10);
    }
} // namespace ex11

// Variations on a theme - std::move and std::move_iterator

namespace ex12
{
    namespace std
    {
        using ::std::conditional_t;
        using ::std::copy;
        using ::std::is_reference_v;
        using ::std::iterator_traits;
        using ::std::remove_reference_t;
        using ::std::string;
        using ::std::vector;

        template <typename T>
        decltype(auto)
        move(T &&t)
        {
            // returns an rvalue (&&)
            return static_cast<remove_reference_t<T> &&>(t);
        }

        template <typename InIt, typename OutIt>
        OutIt
        move(InIt first1, InIt last1, OutIt destination)
        {
            while (first1 != last1)
            {
                *destination = std::move(*first1); // move instead of copy
                ++first1;
                ++destination;
            }
            return destination;
        }

        // wrapper around existing iterator It
        template <typename It>
        class move_iterator
        {
          private:
            using OriginalRefType = typename std::iterator_traits<It>::reference;

            It iter;

          public:
            using iterator_category = typename std::iterator_traits<It>::iterator_category;
            using difference_type   = typename std::iterator_traits<It>::difference_type;
            using value_type        = typename std::iterator_traits<It>::value_type;
            using pointer           = It;
            using reference =
                std::conditional_t<std::is_reference_v<OriginalRefType>,
                                   std::remove_reference_t<OriginalRefType> &&, // make an rvalue out of reference
                                   OriginalRefType>;

            constexpr move_iterator() = default;
            constexpr explicit move_iterator(It it) : iter(std::move(it))
            {
            }

            // Allow constructing or assigning from any kind of move-iterator.
            // These templates also serve as our own type's copy constructor
            // and assignment operator, respectively.
            template <typename U>
            constexpr move_iterator(const move_iterator<U> &m) : iter(m.base())
            {
            }

            template <typename U>
            constexpr auto &
            operator=(const move_iterator<U> &m)
            {
                iter = m.base();
                return *this;
            }

            // returns wrapped-up iterator
            constexpr It
            base() const
            {
                return iter;
            }

            // deref returns an rvalue
            reference
            operator*()
            {
                return static_cast<reference>(*iter);
            }

            It
            operator->()
            {
                return iter;
            }

            constexpr decltype(auto)
            operator[](difference_type n) const
            {
                return *std::move(iter[n]);
            }

            auto &
            operator++()
            {
                ++iter;
                return *this;
            }

            auto &
            operator++(int)
            {
                auto result = *this;
                ++*this;
                return result;
            }

            auto &
            operator--()
            {
                --iter;
                return *this;
            }

            auto &
            operator--(int)
            {
                auto result = *this;
                --*this;
                return result;
            }

            constexpr auto &
            operator+=(difference_type n) const
            {
                iter += n;
                return *this;
            }

            constexpr auto &
            operator-=(difference_type n) const
            {
                iter -= n;
                return *this;
            }
        };

        // I've omitted the definitions of non-member operators
        // == != < <= > >= + - ; can you fill them in?

        // not necessary in practice (just use CTAD)
        template <typename InputIterator>
        auto
        make_move_iterator(InputIterator &c)
        {
            return move_iterator(c);
        }

        template <typename T, class U>
        bool
        operator!=(const move_iterator<T> &t, const move_iterator<U> &u)
        {
            return t.base() != u.base();
        }

        template <typename T>
        bool
        operator-(const move_iterator<T> &t, const move_iterator<T> &u)
        {
            // libstdc++ needs this for std::copy, for whatever reason
            return t.base() - u.base();
        }
    } // namespace std

    void
    test()
    {
        std::vector<std::string> input = {"hello", "world"};
        std::vector<std::string> output(2);

        // First approach: use the std::move algorithm
        std::move(input.begin(),
                  input.end(), //
                  output.begin());

        // Second approach: use copy with move_iterators
        std::copy(std::move_iterator(input.begin()),
                  std::move_iterator(input.end()), //
                  output.begin());
    }
} // namespace ex12

// Complicated copying with std::transform

namespace ex15
{
    void
    test()
    {
        std::vector<const char *> input = {"hello", "world"};
        std::vector<std::string>  output(2);

        // input and output types do not be the same
        // using implicit conversion
        std::copy(input.begin(), input.end(), output.begin());

        assert(output[0] == "hello");
        assert(output[1] == "world");
    }
} // namespace ex15

namespace ex16
{
    template <typename InIt, typename OutIt, typename Unary>
    OutIt
    transform(InIt first1, InIt last1, OutIt destination, Unary op)
    {
        while (first1 != last1)
        {
            *destination = op(*first1); // function with one arg
            ++first1;
            ++destination;
        }
        return destination;
    }

    void
    test()
    {
        std::vector<std::string> input = {"hello", "world"};
        std::vector<std::string> output(2);

        // It works for transforming in-place, too!
        std::transform(input.begin(), input.end(), output.begin(), [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        });

        assert(input[0] == "hello");
        assert(output[0] == "HELLO");
    }
} // namespace ex16

namespace ex17
{
    // could be described as a one-and-two-halves-range algorithm ;-)
    // first1, last1 = one range
    // first2        = half range
    // destination   = half range
    template <typename InIt1, typename InIt2, typename OutIt, typename Binary>
    OutIt
    transform(InIt1 first1, InIt1 last1, InIt2 first2, OutIt destination, Binary op)
    {
        while (first1 != last1)
        {
            *destination = op(*first1, *first2); // function with two args
            ++first1;
            ++first2;
            ++destination;
        }
        return destination;
    }
} // namespace ex17

namespace ex18
{
    void
    test()
    {
        std::vector<std::string> input = {"hello", "world"};
        std::vector<std::string> output(2);

        // Third approach of moving data: use std::transform
        // Not recommended: Whenever you see an explicit specialization - those angle brackets after the template's
        // name - that's an almost sure sign of very subtle and fragile code.
        std::transform(input.begin(),
                       input.end(),    //
                       output.begin(), //
                       std::move<std::string &>);

        assert(input[0] == "");
        assert(input[1] == "");
        assert(output[0] == "hello");
    }
} // namespace ex18

// Write-only range algorithms

namespace ex19
{
    template <typename FwdIt, typename T>
    void
    fill(FwdIt first, FwdIt last, T value)
    {
        while (first != last)
        {
            *first = value;
            ++first;
        }
    }

    template <typename FwdIt, typename T>
    void
    iota(FwdIt first, FwdIt last, T value)
    {
        while (first != last)
        {
            *first = value;
            ++value;
            ++first;
        }
    }

    template <typename FwdIt, typename G>
    void
    generate(FwdIt first, FwdIt last, G generator)
    {
        while (first != last)
        {
            *first = generator();
            ++first;
        }
    }

    void
    test()
    {
        std::vector<std::string> v(4);

        std::fill(v.begin(), v.end(), "hello");
        assert(v[0] == "hello");
        assert(v[1] == "hello");
        assert(v[2] == "hello");
        assert(v[3] == "hello");

        std::iota(v.begin(), v.end(), "hello");
        assert(v[0] == "hello");
        assert(v[1] == "ello");
        assert(v[2] == "llo");
        assert(v[3] == "lo");

        std::generate(v.begin(), v.end(), [i = 0]() mutable { return ++i % 2 ? "hello" : "world"; });
        assert(v[0] == "hello");
        assert(v[1] == "world");
        assert(v[2] == "hello");
        assert(v[3] == "world");
    }
} // namespace ex19

// Algorithms that affect object lifetime

namespace ex37
{
    template <typename T>
    void
    destroy_at(T *p)
    {
        p->~T();
    }

    template <typename FwdIt>
    void
    destroy(FwdIt first, FwdIt last)
    {
        for (; first != last; ++first)
        {
            std::destroy_at(std::addressof(*first));
        }
    }

    template <typename It, typename FwdIt>
    FwdIt
    uninitialized_copy(It first, It last, FwdIt out)
    {
        using T       = typename std::iterator_traits<FwdIt>::value_type;
        FwdIt old_out = out;

        try
        {
            while (first != last)
            {
                ::new (static_cast<void *>(std::addressof(*out))) T(*first);
                ++first;
                ++out;
            }
            return out;
        }
        catch (...)
        {
            std::destroy(old_out, out);
            throw;
        }
    }

    void
    test()
    {
        alignas(std::string) char b[5 * sizeof(std::string)];
        std::string              *sb  = reinterpret_cast<std::string *>(b);
        std::vector<const char *> vec = {"quick", "brown", "fox"};

        // Construct three std::strings.
        auto end = std::uninitialized_copy(vec.begin(), vec.end(), sb);

        assert(end == sb + 3);

        // Destroy three std::strings.
        std::destroy(sb, end);
    }
} // namespace ex37

// Our first permutative algorithm: std::sort

namespace ex21
{
    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9};
        std::sort(v.begin(), v.end(), //
                  [](auto &&a, auto &&b) { return a % 7 < b % 7; });
        assert((v == std::vector{1, 1, 9, 3, 4, 5}));
    }
} // namespace ex21

// Swapping, reversing, and partitioning

namespace ex22
{
    namespace my
    {
        class obj
        {
          private:
            int v;

          public:
            obj(int value) : v(value)
            {
            }

            void
            swap(obj &other)
            {
                using std::swap;
                swap(this->v, other.v);
            }
        };

        void
        swap(obj &a, obj &b)
        {
            a.swap(b);
        }
    } // namespace my

    void
    test()
    {
        using std::swap;

        // calls std::swap<int>(int&, int&)
        int i1 = 1, i2 = 2;
        swap(i1, i2);

        // calls std::swap(vector&, vector&)
        std::vector<int> v1 = {1}, v2 = {2};
        swap(v1, v2);

        // calls my::swap(obj&, obj&)
        my::obj m1 = 1, m2 = 2;
        swap(m1, m2);
    }
} // namespace ex22

namespace ex23
{
    void
    reverse_words_in_place(std::string &s)
    {
        // First, reverse the whole string.
        std::reverse(s.begin(), s.end());

        // Next, un-reverse each individual word.
        for (auto it = s.begin(); true; ++it)
        {
            auto next = std::find(it, s.end(), ' ');
            // Reverse the order of letters in this word.
            std::reverse(it, next);
            if (next == s.end())
            {
                break;
            }
            it = next;
        }
    }

    void
    test()
    {
        std::string s = "the quick brown fox jumps over the lazy dog";
        reverse_words_in_place(s);

        assert(s == "dog lazy the over jumps fox brown quick the");
    }
} // namespace ex23

namespace ex24
{
    namespace std
    {
        using ::std::swap;
        using ::std::vector;

        template <typename BidirIt>
        void
        reverse(BidirIt first, BidirIt last)
        {
            while (first != last)
            {
                --last;
                if (first == last)
                {
                    break;
                }

                using std::swap;
                swap(*first, *last);
                ++first;
            }
        }

        template <typename BidirIt, typename Unary>
        auto
        partition(BidirIt first, BidirIt last, Unary p)
        {
            while (first != last && p(*first))
            {
                ++first;
            }

            while (first != last)
            {
                do
                {
                    --last;
                } while (last != first && !p(*last));

                if (first == last)
                {
                    break;
                }

                using std::swap;
                swap(*first, *last);

                do
                {
                    ++first;
                } while (first != last && p(*first));
            }

            return first;
        }

        void
        test()
        {
            std::vector<int> v  = {3, 1, 4, 1, 5, 9, 2, 6, 5};
            auto             it = std::partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
            assert(it == v.begin() + 3);
            assert((v == std::vector{6, 2, 4, 1, 5, 9, 1, 3, 5}));
        }
    } // namespace std
} // namespace ex24

namespace ex25
{
    namespace std
    {
        using ::std::cout;
        using ::std::find_if;
        using ::std::find_if_not;
        using ::std::reverse_iterator;
        using ::std::swap;
        using ::std::vector;

        //  Shorthands for "reversing" and "unreversing".
        template <typename It>
        auto
        rev(It it)
        {
            return std::reverse_iterator(it);
        };

        template <typename InnerIt>
        auto
        unrev(std::reverse_iterator<InnerIt> it)
        {
            return it.base();
        }

        template <typename BidirIt, typename Unary>
        auto
        partition(BidirIt first, BidirIt last, Unary p)
        {
            first = std::find_if_not(first, last, p);
            // std::cout << "first: " << *first << '\n';

            while (first != last)
            {
                last = unrev(std::find_if(rev(last), rev(first), p)); // both iterators need to be reversed
                // std::cout << "last: " << *last << '\n';
                if (first == last)
                {
                    break;
                }

                using std::swap;
                // std::cout << *first << " " << *last << '\n';
                swap(*first++, *--last); // NOTE: pre-decrement on last

                first = std::find_if_not(first, last, p);
            }
            return first;
        }
    } // namespace std

    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5};

        auto it = std::partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });

        assert(it == v.begin() + 3);
        assert((v == std::vector{6, 2, 4, 1, 5, 9, 1, 3, 5}));
    }
} // namespace ex25

// Rotation and permutation

namespace ex28
{
    namespace std
    {
        using ::std::find;
        using ::std::reverse;
        using ::std::vector;

        template <typename FwdIt>
        FwdIt
        rotate(FwdIt a, FwdIt mid, FwdIt b)
        {
            auto result = a + (b - mid);

            // First, reverse the whole range.
            std::reverse(a, b);

            // Next, un-reverse each individual segment.
            std::reverse(a, result);
            std::reverse(result, b);

            return result;
        }

        void
        test()
        {
            {
                std::vector<int> v = {1, 2, 3, 4, 5, 6};

                auto five = std::find(v.begin(), v.end(), 5);
                auto one  = std::rotate(v.begin(), five, v.end());

                assert((v == std::vector{5, 6, 1, 2, 3, 4}));
                assert(*one == 1);
            }

            {
                std::vector<int> v = {1, 2, 3, 4, 5, 6};

                auto four = std::find(v.begin(), v.end(), 4);
                auto one  = std::rotate(v.begin(), four, v.end());

                assert((v == std::vector{4, 5, 6, 1, 2, 3}));
                assert(*one == 1);
            }
        }
    } // namespace std
} // namespace ex28

namespace ex29
{
    void
    test()
    {

        std::vector<int>              p = {10, 20, 30};
        std::vector<std::vector<int>> results;

        // Collect the permutations of these three elements.
        do
        {
            results.push_back(p);
        } while (std::next_permutation(p.begin(), p.end()));

        assert((results == std::vector<std::vector<int>>{
                               {10, 20, 30},
                               {10, 30, 20},
                               {20, 10, 30},
                               {20, 30, 10},
                               {30, 10, 20},
                               {30, 20, 10},
                           }));
    }
} // namespace ex29

// Heaps and heapsort

namespace ex26
{
#define make_heap MakeHeap
#define push_heap PushHeap
#define pop_heap  PopHeap
#define sort_heap SortHeap

    // https://www.cs.dartmouth.edu/~cs10/notes14.html
    // Heap represented as array:
    //
    // max-heap property: each element of the range at index i
    // will be at least as great as either of the elements at
    // indices 2i+1 and 2i+2
    //
    // This implies that the greatest element of all will be at index 0.
    //
    // prerequisite: range [a,b-1) is already a max-heap
    template <typename RandomIt>
    void
    push_heap(RandomIt a, RandomIt b)
    {
        auto child = ((b - 1) - a);

        while (child != 0)
        {
            auto parent = (child - 1) / 2;
            if (a[child] < a[parent])
            {
                return;                            // max-heap property has been restored
            }
            std::iter_swap(a + child, a + parent); // swaps elements at the two iterators
            child = parent;
        }
    }

    template <typename RandomIt>
    void
    make_heap(RandomIt a, RandomIt b)
    {
        for (auto it = a; it != b;)
        {
            push_heap(a, ++it);
        }
    }

    template <typename RandomIt>
    void
    pop_heap(RandomIt a, RandomIt b)
    {
        using DistanceT = decltype(b - a);

        std::iter_swap(a, b - 1);

        DistanceT parent        = 0;
        DistanceT new_heap_size = ((b - 1) - a);

        while (true)
        {
            auto leftchild  = 2 * parent + 1;
            auto rightchild = 2 * parent + 2;
            if (leftchild >= new_heap_size)
            {
                return;
            }

            auto biggerchild = leftchild;
            if (rightchild < new_heap_size && a[leftchild] < a[rightchild])
            {
                biggerchild = rightchild;
            }

            if (a[biggerchild] < a[parent])
            {
                return; // max-heap property has been restored
            }

            std::iter_swap(a + parent, a + biggerchild);
            parent = biggerchild;
        }
    }

    template <typename RandomIt>
    void
    sort_heap(RandomIt a, RandomIt b)
    {
        for (auto it = b; it != a; --it)
        {
            pop_heap(a, it);
        }
    }

    template <typename RandomIt>
    void
    sort(RandomIt a, RandomIt b)
    {
        make_heap(a, b);
        sort_heap(a, b);
    }

    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5};

        ex26::sort(v.begin(), v.end());
        assert((v == std::vector{1, 1, 2, 3, 4, 5, 5, 6, 9}));
    }

#undef make_heap
#undef push_heap
#undef pop_heap
#undef sort_heap
} // namespace ex26

// Merges and mergesort

namespace ex27
{
    namespace std
    {
        using ::std::distance;
        using ::std::inplace_merge;
        using ::std::vector;

        template <class Iter>
        void
        merge_sort(Iter a, Iter b)
        {
            if (b - a > 1)
            {
                Iter middle = a + (b - a) / 2;
                merge_sort(a, middle);
                merge_sort(middle, b);
                std::inplace_merge(a, middle, b); // inplace_merge function allocates a buffer for its own use!!!
                                                  // std::merge(a,b,c,d,o) is the non-allocating merge algorithm
            }
        }

        template <typename RandomIt>
        void
        sort(RandomIt a, RandomIt b)
        {
            auto n = std::distance(a, b);
            if (n >= 2)
            {
                auto mid = a + n / 2;
                std::sort(a, mid);
                std::sort(mid, b);
                std::inplace_merge(a, mid, b);
            }
        }
    } // namespace std

    void
    test()
    {
        {
            std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5};
            std::sort(v.begin(), v.end());
            assert((v == std::vector{1, 1, 2, 3, 4, 5, 5, 6, 9}));
        }

        {
            std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5};
            std::merge_sort(v.begin(), v.end());
            assert((v == std::vector{1, 1, 2, 3, 4, 5, 5, 6, 9}));
        }
    }
} // namespace ex27

// Searching and inserting in a sorted array with std::lower_bound

namespace ex30
{
    // The standard algorithm that implements binary search is called `lower_bound`.
    // Prerequisite: already sorted range (according to cmp - typically <=).
    // Returns an iterator to the first element in the range that is not less than the given `value`.
    // Will point at the place where `value` should go, if you want to insert it.
    template <typename FwdIt, typename T, typename C>
    FwdIt
    lower_bound(FwdIt first, FwdIt last, const T &value, C cmp)
    {
        using DiffT = typename std::iterator_traits<FwdIt>::difference_type;

        FwdIt it;
        DiffT count = std::distance(first, last);

        while (count > 0)
        {
            DiffT step = count / 2; // do a binary search

            it = first;
            std::advance(it, step); // it += step
            if (cmp(*it, value))
            {
                ++it;
                first = it;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }
        return first;
    }

    template <typename FwdIt, typename T>
    FwdIt
    lower_bound(FwdIt first, FwdIt last, const T &value)
    {
        return std::lower_bound(first, last, value, std::less<>{});
    }

    void
    test()
    {
        {
            std::vector<int> v = {1, 2, 3, 4, 6, 7, 8, 9};

            auto it = ex30::lower_bound(v.begin(), v.end(), 5);
            assert(*it == 6);
            assert(it == v.begin() + 4);
        }

        {
            std::vector<std::string> v = {"hello", "world"};

            auto it = ex30::lower_bound(v.begin(), v.end(), "literally");
            assert(it == v.begin() + 1);
        }
    }
} // namespace ex30

namespace ex31
{
    void
    test()
    {
        std::vector<int> vec = {3, 7};

        // inserting values
        for (int value : {1, 5, 9})
        {
            // Find the appropriate insertion point...
            auto it = std::lower_bound(vec.begin(), vec.end(), value);
            // ...and insert our value there.
            vec.insert(it, value);
        }

        // The vector has remained sorted.
        assert((vec == std::vector{1, 3, 5, 7, 9}));
    }
} // namespace ex31

namespace ex32
{
    void
    test()
    {
        // std::lower_bound and std::upper_bound will give you a half-open range [lower, upper)
        // containing nothing but instances of the given value.

        std::vector<int> vec = {2, 3, 3, 3, 4};

        auto lower = std::lower_bound(vec.begin(), vec.end(), 3);

        // First approach:
        // upper_bound's interface is identical to lower_bound's.
        auto upper = std::upper_bound(vec.begin(), vec.end(), 3);
        assert(*lower == 3);
        assert(*upper == 4);

        // Second approach:
        // We don't need to binary-search the whole array the second time.
        auto upper2 = std::upper_bound(lower, vec.end(), 3);
        assert(upper2 == upper);

        // Third approach:
        // Linear scan from the lower bound might well be faster
        // than binary search if our total range is really big.
        auto upper3 = std::find_if(lower, vec.end(), [](int v) { return v != 3; });
        assert(upper3 == upper);

        // No matter which approach we take, this is what we end up with.
        assert(*lower >= 3);
        assert(*upper > 3);
        assert(std::all_of(lower, upper, [](int v) { return v == 3; }));
    }
} // namespace ex32

// Deleting from a sorted array with std::remove_if

// So how do we erase items from a range? Well, we can't! All we can do is erase items from a
// container; and the algorithms of the STL do not deal in containers. So what we ought to be
// looking for is a way to rearrange the values of a range so that the "removed" items will wind
// up somewhere predictable, so that we can quickly erase them all from the underlying
// container (using some means other than an STL algorithm).
//
// Leads to the ERASE-REMOVE-IDIOM!

namespace ex33
{
    void
    test()
    {
        std::vector<int> vec = {1, 3, 3, 4, 6, 8};

        // Partition our vector so that all the non-3s are at the front
        // and all the 3s are at the end.
        auto first_3 = std::stable_partition(vec.begin(), vec.end(), [](int v) { return v != 3; });

        assert((vec == std::vector{1, 4, 6, 8, 3, 3}));

        // Now erase the "tail" of our vector.
        vec.erase(first_3, vec.end());

        assert((vec == std::vector{1, 4, 6, 8}));

        // BUT!!! stable_partition is one of those few STL algorithms that allocates a temporary buffer on the heap!
    }
} // namespace ex33

namespace ex34
{
    template <typename FwdIt, typename T>
    FwdIt
    remove(FwdIt first, FwdIt last, const T &value)
    {
        auto out = std::find(first, last, value);
        if (out != last)
        {
            auto in = out;
            while (++in != last)
            {
                if (*in == value)
                {
                    // don't bother with this item
                }
                else
                {
                    *out++ = std::move(*in);
                }
            }
        }
        return out;
    }

    void
    test()
    {
        std::vector<int> vec = {1, 3, 3, 4, 6, 8};

        // Partition our vector so that all the non-3s are at the front.
        auto new_end = std::remove(vec.begin(), vec.end(), 3);

        // std::remove_if doesn't preserve the "removed" elements.
        assert((vec == std::vector{1, 4, 6, 8, 6, 8}));

        // Now erase the "tail" of our vector.
        vec.erase(new_end, vec.end());

        assert((vec == std::vector{1, 4, 6, 8}));

        // Or, do both steps together in a single line.
        // This is the "erase-remove idiom":
        vec.erase(std::remove(vec.begin(), vec.end(), 3), vec.end());

        // But if the array is very long, and we know it's sorted,
        // then perhaps it would be better to binary-search for
        // the elements to erase.
        // Here the "shifting-down" is still happening, but it's
        // happening inside vector::erase instead of inside std::remove.
        auto first = std::lower_bound(vec.begin(), vec.end(), 3);
        auto last  = std::upper_bound(first, vec.end(), 3);
        vec.erase(first, last);
    }
} // namespace ex34

namespace ex35
{
    void
    test()
    {
        std::vector<int> vec = {1, 2, 2, 3, 3, 3, 1, 3, 3};

        // std::unique(a,b) takes a range and for each set of consecutive equivalent items
        // removes all but the first of them.
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

        assert((vec == std::vector{1, 2, 3, 1, 3}));
    }
} // namespace ex35

namespace ex36
{
    namespace my
    {
        template <typename BidirIt, typename T>
        BidirIt
        unstable_remove(BidirIt first, BidirIt last, const T &value)
        {
            while (true)
            {
                // Find the first instance of "value"...
                first = std::find(first, last, value);
                // ...and the last instance of "not value"...
                do
                {
                    if (first == last)
                    {
                        return last;
                    }
                    --last;
                } while (*last == value);
                // ...and move the latter over top of the former.
                *first = std::move(*last);
                // Rinse and repeat.
                ++first;
            }
        }
    } // namespace my

    void
    test()
    {
        std::vector<int> vec = {4, 1, 3, 6, 3, 8};

        vec.erase(my::unstable_remove(vec.begin(), vec.end(), 3), vec.end());

        assert((vec == std::vector{4, 1, 8, 6}));
    }
} // namespace ex36

int
main()
{
    ex01::test();
    ex02::test();
    ex03::test();
    ex06::test();
    ex07::test();
    ex09::test();
    ex11::test();
    ex12::test();
    ex15::test();
    ex18::test();
    ex19::test();
    ex21::test();
    ex22::test();
    ex23::test();
    ex24::std::test();
    ex25::test();
    ex26::test();
    ex27::test();
    ex28::std::test();
    ex29::test();
    ex30::test();
    ex31::test();
    ex32::test();
    ex33::test();
    ex34::test();
    ex35::test();
    ex36::test();
    ex37::test();
}
