#include <cassert>
#include <stdexcept>
#include <vector>

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"

// concrete (monomorphic) functions
namespace ex1
{
    class array_of_ints
    {
        int data[10] = {};

      public:
        int
        size() const
        {
            return 10;
        }
        int &
        at(int i)
        {
            return data[i];
        }
    };

    // monomorphic function
    void
    double_each_element(array_of_ints &arr)
    {
        for (int i = 0; i < arr.size(); ++i)
        {
            arr.at(i) *= 2;
        }
    }

    void
    test()
    {
        array_of_ints arr;
        double_each_element(arr);

        // std::vector<int> vec = {1, 2, 3};
        // double_each_element(vec);
    }
} // namespace ex1

// classically polymorphic functions
namespace ex2
{
    class container_of_ints
    {
      public:
        virtual int  size() const = 0;
        virtual int &at(int)      = 0;
    };

    class array_of_ints : public container_of_ints
    {
      public:
        int
        size() const override
        {
            return 10;
        }

        int &
        at(int i) override
        {
            return data[i];
        }

      private:
        int data[10] = {};
    };

    class list_of_ints : public container_of_ints
    {
      public:
        int
        size() const override
        {
            return size_;
        }

        int &
        at(int i) override
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
        int   size_ = 0;
    };

    // polymorphic function
    void
    double_each_element(container_of_ints &arr)
    {
        for (int i = 0; i < arr.size(); ++i)
        {
            arr.at(i) *= 2;
        }
    }

    void
    test()
    {
        array_of_ints arr;
        double_each_element(arr);

        list_of_ints lst;
        double_each_element(lst);

        // std::vector<int> vec = {1, 2, 3};
        // double_each_element(vec);
    }
} // namespace ex2

// generic programming
namespace ex3
{
    class container_of_ints
    {
      public:
        virtual int  size() const = 0;
        virtual int &at(int)      = 0;
    };

    class array_of_ints : public container_of_ints
    {
      public:
        int
        size() const override
        {
            return 10;
        }

        int &
        at(int i) override
        {
            return data[i];
        }

      private:
        int data[10] = {};
    };

    class list_of_ints : public container_of_ints
    {
      public:
        int
        size() const override
        {
            return size_;
        }

        int &
        at(int i) override
        {
            node *p = head_;

            for (int j = 0; p != nullptr && j < i; ++j)
            {
                p = p->next;
            }

            return *(p ? &p->data : throw std::out_of_range("at"));
        }

      private:
        struct node
        {
            int   data;
            node *next;
        };

        node *head_ = nullptr;
        int   size_ = 0;
    };

    // generic function
    // `ContainerModel` is a model of the `Container` concept.
    template <typename ContainerModel>
    void
    double_each_element(ContainerModel &cm)
    {
        for (int i = 0; i < cm.size(); ++i)
        {
            cm.at(i) *= 2;
        }
    }

    void
    test()
    {
        array_of_ints arr;
        double_each_element(arr);

        list_of_ints lst;
        double_each_element(lst);

        std::vector<int> vec = {1, 2, 3};
        double_each_element(vec);

        std::vector<double> vecd = {1.0, 2.0, 3.0};
        double_each_element(vecd);
    }
} // namespace ex3

// more examples of generic algorithms
namespace ex4
{
    template <typename Container>
    int
    count(const Container &container)
    {
        int sum = 0;

        for (auto &&elt : container)
        {
            sum += 1;
        }

        return sum;
    }

    template <typename Container, typename Predicate>
    int
    count_if(const Container &container, Predicate pred)
    {
        int sum = 0;

        for (auto &&elt : container) // forwarding reference takes everything
        {
            // we never need to specify whether pred takes its argument by value or by reference
            if (pred(elt))
            {
                sum += 1;
            }
        }

        return sum;
    }

    void
    test()
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

        assert(count(v) == 8);

        int number_above = count_if(v, [](auto elem) { return elem > 5; });
        int number_below = count_if(v, [](auto elem) { return elem < 5; });

        assert(number_above == 2);
        assert(number_below == 5);
    }
} // namespace ex4

int
main()
{
    ex1::test();
    ex2::test();
    ex3::test();
    ex4::test();
}
