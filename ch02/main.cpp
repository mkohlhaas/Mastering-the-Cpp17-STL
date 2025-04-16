#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <vector>

#pragma GCC diagnostic ignored "-Wsign-compare"

// The problem with integer indices

namespace ex1
{
    template <typename Container>
    void
    double_each_element(Container &c)
    {
        for (int i = 0; i < c.size(); ++i)
        {
            c.at(i) *= 2;
        }
    }

    class list_of_ints
    {
      public:
        int
        size() const
        {
            return size_;
        }

        int &
        at(int i)
        {
            if (i >= size_)
            {
                throw std::out_of_range("at");
            }

            node *p = head_;
            for (int j = 0; j < i; ++j)
            {
                p = p->next;
            }
            return p->data;
        }

        void
        push_back(int value)
        {
            node *new_tail = new node{value, nullptr};
            if (tail_)
            {
                tail_->next = new_tail;
            }
            else
            {
                head_ = new_tail;
            }

            tail_ = new_tail;
            size_ += 1;
        }

        ~list_of_ints()
        {
            for (node *next, *p = head_; p != nullptr; p = next)
            {
                next = p->next;
                delete p;
            }
        }

      private:
        struct node
        {
            int   data;
            node *next;
        };

        node *head_ = nullptr;
        node *tail_ = nullptr;
        int   size_ = 0;
    };

    void
    test()
    {
        std::vector<int> v;
        double_each_element(v);

        list_of_ints lst;
        lst.push_back(1);
        assert(lst.at(0) == 1);
        lst.push_back(2);
        lst.push_back(3);

        // .at - which goes through the list - is called every time we want to advance by one element!
        // O(n^2) instead of O(n) !
        double_each_element(lst);

        assert(lst.at(0) == 2);
        assert(lst.at(1) == 4);
        assert(lst.at(2) == 6);
    }
} // namespace ex1

// On beyond pointers

// iterating over a linked list
namespace ex3
{
    struct node
    {
        int   data;
        node *next;
    };

    struct list_of_ints
    {
        node *head_ = nullptr;
    };

    void
    test()
    {
        list_of_ints lst;
        int          sum  = 0;
        auto         pred = [](int) { return true; };

        for (node *p = lst.head_; p != nullptr; p = p->next)
        {
            if (pred(p->data))
            {
                sum += 1;
            }
        }

        std::cout << "sum: " << sum << '\n'; // sum: 0
    }
} // namespace ex3

// overloading ++(increment) and *(deref) operators
namespace ex4
{
    struct list_node
    {
        int        data;
        list_node *next;
    };

    class int_list_iter
    {
      private:
        friend class list_of_ints;

        list_node *ptr_;

        explicit int_list_iter(list_node *p) : ptr_(p)
        {
        }

      public:
        // deref
        int &
        operator*() const
        {
            return ptr_->data;
        }

        // prefix increment (++i)
        int_list_iter &
        operator++()
        {
            ptr_ = ptr_->next;
            return *this;
        }

        // postfix increment (i++)
        int_list_iter
        operator++(int)
        {
            auto result = *this;
            ++*this;
            return result;
        }

        // we need to compare begin and end in the iterator eg. for count_if
        bool
        operator!=(const int_list_iter &rhs) const
        {
            return ptr_ != rhs.ptr_;
        }

        // we define this bc we have to define !=
        bool
        operator==(const int_list_iter &rhs) const
        {
            return ptr_ == rhs.ptr_;
        }
    };

    class list_of_ints
    {
      public:
        using iterator = int_list_iter;

        iterator
        begin()
        {
            return iterator{head_};
        }

        iterator
        end()
        {
            return iterator{nullptr};
        }

      private:
        list_node *head_ = nullptr;
    };

    // count_if needs ++, *, and !=
    template <class Container, class Predicate>
    int
    count_if(Container &ctr, Predicate pred)
    {
        int sum = 0;

        for (auto it = ctr.begin(); it != ctr.end(); ++it)
        {
            if (pred(*it))
            {
                sum += 1;
            }
        }

        return sum;
    }

    void
    test()
    {
        list_of_ints lst;
        int          s = count_if(lst, [](int &i) { return i > 5; });

        assert(s == 0);
    }
} // namespace ex4

// Const iterators

// const-correct iterator types
namespace ex5
{
    struct list_node
    {
        int        data;
        list_node *next;
    };

    template <bool Const>
    class int_list_iter
    {
      private:
        friend class list_of_ints;
        friend class int_list_iter<!Const>;

        using node_ptr  = std::conditional_t<Const, const list_node *, list_node *>;
        using reference = std::conditional_t<Const, const int &, int &>;

        node_ptr ptr_;

        explicit int_list_iter(node_ptr p) : ptr_(p)
        {
        }

      public:
        reference
        operator*() const
        {
            return ptr_->data;
        }

        auto &
        operator++()
        {
            ptr_ = ptr_->next;
            return *this;
        }

        auto
        operator++(int)
        {
            auto result = *this;
            ++*this;
            return result;
        }

        // support comparison between iterator and const_iterator types
        template <bool R>
        bool
        operator==(const int_list_iter<R> &rhs) const
        {
            return ptr_ == rhs.ptr_;
        }

        template <bool R>
        bool
        operator!=(const int_list_iter<R> &rhs) const
        {
            return ptr_ != rhs.ptr_;
        }

        // casting operator (); cast to const_iterator
        // implicit conversion of iterator to const_iterator (but not vice versa)
        operator int_list_iter<true>() const
        {
            return int_list_iter<true>{ptr_};
        }
    };

    class list_of_ints
    {
      private:
        list_node *head_ = nullptr;

      public:
        using iterator       = int_list_iter<false>;
        using const_iterator = int_list_iter<true>;

        iterator
        begin()
        {
            return iterator{head_};
        }

        iterator
        end()
        {
            return iterator{nullptr};
        }

        const_iterator
        begin() const
        {
            return const_iterator{head_};
        }

        const_iterator
        end() const
        {
            return const_iterator{nullptr};
        }
    };

    void
    test()
    {
        list_of_ints                 lst;
        list_of_ints::iterator       it  = lst.begin();
        list_of_ints::const_iterator itc = lst.begin();

        // implicit convsersion to const allowed
        itc = it;

        // but not other direction
        // it  = itc;

        assert(it == it);
        assert(it == itc);
    }
} // namespace ex5

// A pair of iterators defines a range

// Will eventually lead us to the concept of a NON-OWNING VIEW,
// which is to a data sequence as a C++ reference is to a single variable.
// Non-owning view is a reference to a data sequence, eg. string view.

// Iterator can be any type that implements INCREMENTABILITY, COMPARABILITY, and DEREFERENCEABILITY.
// You can view iterators are abstract pointers.

namespace ex6
{
    template <class Iterator>
    void
    double_each_element(Iterator begin, Iterator end)
    {
        for (auto it = begin; it != end; ++it)
        {
            *it *= 2;
        }
    }

    void
    test()
    {
        std::vector<int> v{1, 2, 3, 4, 5, 6};

        // using iterators
        double_each_element(v.begin(), v.end());       // double each element in the entire vector
        double_each_element(v.begin(), v.begin() + 3); // double each element in the first half

        // Also works fine with pointers! (But don't use it in practice!)
        double_each_element(&v[0], &v[3]); // double each element in the first half
    }
} // namespace ex6

// Iterator categories

// Using iterators now instead of containers.

namespace ex7
{
// rename STL functions
#define count_if count_if_
#define distance distance_

    template <typename Iterator, typename Predicate>
    int
    count_if(Iterator begin, Iterator end, Predicate pred)
    {
        int sum = 0;

        for (auto it = begin; it != end; ++it)
        {
            if (pred(*it))
            {
                sum += 1;
            }
        }

        return sum;
    }

    template <typename Iterator>
    int
    distance(Iterator begin, Iterator end)
    {
        int res = 0;

        for (auto it = begin; it != end; ++it)
        {
            res += 1;
        }

        return res;
    }

    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

        int number_above = count_if(v.begin(), v.end(), [](int e) { return e > 5; });
        int number_below = count_if(v.begin(), v.end(), [](int e) { return e < 5; });

        int total = distance(v.begin(), v.end()); // DUBIOUS

        assert(number_above == 2);
        assert(number_below == 5);
        assert(total == 8);
    }

#undef count_if
#undef distance
} // namespace ex7

namespace ex8
{
    template <typename Iterator, typename Predicate>
    int
    my_count_if(Iterator begin, Iterator end, Predicate pred)
    {
        int sum = 0;

        for (auto it = begin; it != end; ++it)
        {
            if (pred(*it))
            {
                sum += 1;
            }
        }

        return sum;
    }

    template <typename Iterator>
    int
    my_distance(Iterator begin, Iterator end)
    {
        int res = 0;

        for (auto it = begin; it != end; ++it)
        {
            res += 1;
        }

        return res;
    }

    // full specialization (for pointers)
    template <>
    int
    my_distance(int *begin, int *end)
    {
        return end - begin; // pointer arithmetic (much better performance)
    }

    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

        int number_above = my_count_if(v.begin(), v.end(), [](int e) { return e > 5; });
        int number_below = my_count_if(v.begin(), v.end(), [](int e) { return e < 5; });

        int total = my_distance(v.begin(), v.end()); // DUBIOUS

        // We don't want the specialization to exist only for int* and std::vector::iterator.
        // We want the standard library's std::distance to be efficient for all the iterator types that
        // support this particular operation

        // There are (at least) two different kinds of iterators: there are those that are incrementable,
        // comparable, and dereferenceable; and then there are those that are incrementable,
        // comparable, dereferenceable, and also SUBTRACTABLE!

        // STL introduces the idea of a hierarchy of iterator kinds:
        // - RandomAccessIterator  (++, *, !=, --, +, -)
        // - BidirectionalIterator (++, *, !=, --)
        // - ForwardIterator       (++, *, !=)
        // - InputIterator         (++, *, !=)
        // - OutputIterator        (++, *, !=)

        // At compile-time the compiler can't distinguish between the last three iterator types.
        // We have to specify the iterator category (see below).

        assert(number_above == 2);
        assert(number_below == 5);
        assert(total == 8);
    }
} // namespace ex8

// Input and output iterators

namespace ex9
{
    // input iterator
    class getc_iterator
    {
      private:
        char ch;

      public:
        getc_iterator() : ch(getc(stdin))
        {
        }

        char
        operator*() const
        {
            return ch;
        }

        // get another char
        auto &
        operator++()
        {
            ch = getc(stdin);
            return *this;
        }

        auto
        operator++(int)
        {
            auto result(*this);
            ++*this;
            return result;
        }

        // never equal
        bool
        operator==(const getc_iterator &) const
        {
            return false;
        }

        // always unequal
        bool
        operator!=(const getc_iterator &) const
        {
            return true;
        }
    };

    // output iterator
    class putc_iterator
    {
      private:
        struct proxy
        {
            // print char to stdout
            void
            operator=(char ch)
            {
                putc(ch, stdout);
            }
        };

      public:
        proxy
        operator*() const
        {
            return proxy{};
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

        // never equal
        bool
        operator==(const putc_iterator &) const
        {
            return false;
        }

        // always unequal
        bool
        operator!=(const putc_iterator &) const
        {
            return true;
        }
    };

    void
    test()
    {
        putc_iterator it;

        for (char ch : {'h', 'e', 'l', 'l', 'o', '\n'}) // hello
        {
            *it++ = ch;
        }
    }
} // namespace ex9

// specifying the iterator category

namespace ex11
{
    struct list_node
    {
    };

    class getc_iterator
    {
        [[maybe_unused]] char ch;

      public:
        using iterator_category = std::input_iterator_tag; // here

        // ...
    };

    class putc_iterator
    {
        struct proxy
        {
            void
            operator=(char ch)
            {
                putc(ch, stdout);
            }
        };

      public:
        using iterator_category = std::output_iterator_tag; // here

        // ...
    };

    template <bool Const>
    class list_of_ints_iterator
    {
        using node_pointer = std::conditional_t<Const, const list_node *, list_node *>;
        node_pointer ptr_;

      public:
        using iterator_category = std::forward_iterator_tag; // here

        // ...
    };
} // namespace ex11

// The conceptual hierarchy of iterator kinds is reflected in the class hierarchy of iterator_category tag classes:
namespace ex12
{
    struct input_iterator_tag
    {
    };

    struct output_iterator_tag
    {
    };

    struct forward_iterator_tag : public input_iterator_tag
    {
    };

    struct bidirectional_iterator_tag : public forward_iterator_tag
    {
    };

    struct random_access_iterator_tag : public bidirectional_iterator_tag
    {
    };
} // namespace ex12

namespace ex13
{
    void
    foo(std::bidirectional_iterator_tag t [[maybe_unused]])
    {
        puts("std::vector's iterators are indeed bidirectional...");
    }

    void
    bar(std::random_access_iterator_tag)
    {
        puts("...and random-access, too!");
    }

    void
    bar(std::forward_iterator_tag)
    {
        puts("forward_iterator_tag is not as good a match");
    }

    void
    test()
    {
        using It = std::vector<int>::iterator;
        foo(It::iterator_category{});
        bar(It::iterator_category{});
    }
} // namespace ex13

// Putting it all together

// How can we provide a member typedef to something that isn't a class type at all, but rather a primitive scalar type?

// Adding a layer of indirection. Rather than referring directly to T::iterator_category, the standard algorithms are
// careful always to refer to std::iterator_traits<T>::iterator_category. The class template std::iterator_traits<T> is
// appropriately specialized for the case where T is a pointer type.

// Furthermore, std::iterator_traits<T> proved to be a convenient place to hang other
// member typedefs (see below).

namespace ex14
{
    struct list_node
    {
        int        data;
        list_node *next;
    };

    template <bool Const>
    class list_of_ints_iterator
    {
      private:
        friend class list_of_ints;
        friend class list_of_ints_iterator<!Const>;

        using node_pointer = std::conditional_t<Const, const list_node *, list_node *>;

        node_pointer ptr_;

        explicit list_of_ints_iterator(node_pointer p) : ptr_(p)
        {
        }

      public:
        // member typedefs required by std::iterator_traits
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = int;
        using pointer           = std::conditional_t<Const, const int *, int *>;
        using reference         = std::conditional_t<Const, const int &, int &>;

        reference
        operator*() const
        {
            return ptr_->data;
        }

        auto &
        operator++()
        {
            ptr_ = ptr_->next;
            return *this;
        }

        auto
        operator++(int)
        {
            auto result = *this;
            ++*this;
            return result;
        }

        // support comparison between iterator and const_iterator types
        template <bool R>
        bool
        operator==(const list_of_ints_iterator<R> &rhs) const
        {
            return ptr_ == rhs.ptr_;
        }

        template <bool R>
        bool
        operator!=(const list_of_ints_iterator<R> &rhs) const
        {
            return ptr_ != rhs.ptr_;
        }

        // support implicit conversion of iterator to const_iterator (but not vice versa)
        operator list_of_ints_iterator<true>() const
        {
            return list_of_ints_iterator<true>{ptr_};
        }
    };

    class list_of_ints
    {
        list_node *head_ = nullptr;
        list_node *tail_ = nullptr;
        int        size_ = 0;

      public:
        using const_iterator = list_of_ints_iterator<true>;
        using iterator       = list_of_ints_iterator<false>;

        // begin and end member functions
        iterator
        begin()
        {
            return iterator{head_};
        }

        iterator
        end()
        {
            return iterator{nullptr};
        }

        const_iterator
        begin() const
        {
            return const_iterator{head_};
        }

        const_iterator
        end() const
        {
            return const_iterator{nullptr};
        }

        // other member operations
        int
        size() const
        {
            return size_;
        }

        void
        push_back(int value)
        {
            list_node *new_tail = new list_node{value, nullptr};

            if (tail_)
            {
                tail_->next = new_tail;
            }
            else
            {
                head_ = new_tail;
            }

            tail_ = new_tail;
            size_ += 1;
        }

        ~list_of_ints()
        {
            for (list_node *next, *p = head_; p != nullptr; p = next)
            {
                next = p->next;
                delete p;
            }
        }
    };

    template <typename Iterator>
    auto
    distance(Iterator begin, Iterator end)
    {
        using Traits = std::iterator_traits<Iterator>;

        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, typename Traits::iterator_category>)
        {
            return (end - begin);
        }
        else
        {
            auto res = typename Traits::difference_type{};
            for (auto it = begin; it != end; ++it)
            {
                ++res;
            }
            return res;
        }
    }

    template <typename Iterator, typename Predicate>
    auto
    count_if(Iterator begin, Iterator end, Predicate pred)
    {
        using Traits = std::iterator_traits<Iterator>;

        auto sum = typename Traits::difference_type{}; // {} -> initializing difference_type
        for (auto it = begin; it != end; ++it)
        {
            if (pred(*it))
            {
                ++sum;
            }
        }
        return sum;
    }

    void
    test()
    {
        list_of_ints lst;
        lst.push_back(1);
        lst.push_back(2);
        lst.push_back(3);

        int s = count_if(lst.begin(), lst.end(), [](int i) { return i >= 2; });
        assert(s == 2);

        int d = distance(lst.begin(), lst.end());
        assert(d == 3);
    }
} // namespace ex14

// The deprecated std::iterator

namespace ex15
{
    namespace std
    {
        using ::std::forward_iterator_tag;
        using ::std::ptrdiff_t;
    } // namespace std

    namespace std
    {
        template <class Category, class T, class Distance = std::ptrdiff_t, class Pointer = T *, class Reference = T &>
        struct iterator
        {
            using iterator_category = Category;
            using value_type        = T;
            using difference_type   = Distance;
            using pointer           = Pointer;
            using reference         = Reference;
        };
    } // namespace std

    class list_of_ints_iterator : public std::iterator<std::forward_iterator_tag, int>
    {
        // ...
    };
} // namespace ex15

namespace ex16
{
    struct list_node;

    template <bool Const, class Base = std::iterator<std::forward_iterator_tag, int, std::ptrdiff_t,
                                                     std::conditional_t<Const, const int *, int *>,
                                                     std::conditional_t<Const, const int &, int &>>>
    class list_of_ints_iterator : public Base
    {
        using typename Base::reference; // Awkward!

        using node_pointer = std::conditional_t<Const, const list_node *, list_node *>;
        node_pointer ptr_;

      public:
        reference
        operator*() const
        {
            return ptr_->data;
        }
        // ...
    };

    template <typename... Ts, typename Predicate>
    int count_if(const std::iterator<Ts...> &begin, const std::iterator<Ts...> &end, Predicate pred);
} // namespace ex16

// Boost equivalent of std::iterator is boost::iterator_facade (not obsolete)
namespace ex18
{
    struct list_node
    {
        int        data;
        list_node *next;
    };

#include <boost/iterator/iterator_facade.hpp>

    template <bool Const>
    class list_of_ints_iterator : public boost::iterator_facade<list_of_ints_iterator<Const>,              //
                                                                std::conditional_t<Const, const int, int>, //
                                                                std::forward_iterator_tag>                 //
    {
      private:
        friend class boost::iterator_core_access;
        friend class list_of_ints;
        friend class list_of_ints_iterator<!Const>;

        using node_pointer = std::conditional_t<Const, const list_node *, list_node *>;
        node_pointer ptr_;

        explicit list_of_ints_iterator(node_pointer p) : ptr_(p)
        {
        }

        auto &
        dereference() const
        {
            return ptr_->data;
        }
        void
        increment()
        {
            ptr_ = ptr_->next;
        }

        // support comparison between iterator and const_iterator types
        template <bool R>
        bool
        equal(const list_of_ints_iterator<R> &rhs) const
        {
            return ptr_ == rhs.ptr_;
        }

      public:
        // support implicit conversion of iterator to const_iterator (but not vice versa)
        operator list_of_ints_iterator<true>() const
        {
            return list_of_ints_iterator<true>{ptr_};
        }
    };

    class list_of_ints
    {
        list_node *head_ = nullptr;
        list_node *tail_ = nullptr;
        int        size_ = 0;

      public:
        using const_iterator = list_of_ints_iterator<true>;
        using iterator       = list_of_ints_iterator<false>;

        // begin and end member functions
        iterator
        begin()
        {
            return iterator{head_};
        }

        iterator
        end()
        {
            return iterator{nullptr};
        }

        const_iterator
        begin() const
        {
            return const_iterator{head_};
        }

        const_iterator
        end() const
        {
            return const_iterator{nullptr};
        }

        // other member operations
        int
        size() const
        {
            return size_;
        }

        void
        push_back(int value)
        {
            list_node *new_tail = new list_node{value, nullptr};
            if (tail_)
            {
                tail_->next = new_tail;
            }
            else
            {
                head_ = new_tail;
            }
            tail_ = new_tail;
            size_ += 1;
        }

        ~list_of_ints()
        {
            for (list_node *next, *p = head_; p != nullptr; p = next)
            {
                next = p->next;
                delete p;
            }
        }
    };

    template <typename Iterator>
    auto
    distance(Iterator begin, Iterator end)
    {
        using Traits = std::iterator_traits<Iterator>;

        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, typename Traits::iterator_category>)
        {
            return (end - begin);
        }
        else
        {
            auto result = typename Traits::difference_type{};
            for (auto it = begin; it != end; ++it)
            {
                ++result;
            }
            return result;
        }
    }

    template <typename Iterator, typename Predicate>
    auto
    count_if(Iterator begin, Iterator end, Predicate pred)
    {
        using Traits = std::iterator_traits<Iterator>;

        auto sum = typename Traits::difference_type{};
        for (auto it = begin; it != end; ++it)
        {
            if (pred(*it))
            {
                ++sum;
            }
        }
        return sum;
    }

    void
    test()
    {
        list_of_ints lst;
        assert(lst.begin() == lst.end());

        lst.push_back(1);
        assert(lst.begin() != std::cend(lst));

        lst.push_back(2);
        lst.push_back(3);
        int s = count_if(lst.begin(), lst.end(), [](int i) { return i >= 2; });
        assert(s == 2);

        s = count_if(std::cbegin(lst), std::cend(lst), [](int i) { return i >= 2; });
        assert(s == 2);

        s = std::count_if(lst.begin(), lst.end(), [](int i) { return i >= 2; });
        assert(s == 2);

        s = std::count_if(std::cbegin(lst), std::cend(lst), [](int i) { return i >= 2; });
        assert(s == 2);

        int d = distance(lst.begin(), lst.end());
        assert(d == 3);
    }
} // namespace ex18

int
main()
{
    ex1::test();
    ex3::test();
    ex4::test();
    ex5::test();
    ex6::test();
    ex7::test();
    ex8::test();
    ex9::test();
    ex14::test();
    ex18::test();
}
