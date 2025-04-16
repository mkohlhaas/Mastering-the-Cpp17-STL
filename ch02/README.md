[std::iterator is deprecated: Why, What It Was, and What to Use Instead](https://www.fluentcpp.com/2018/05/08/std-iterator-deprecated/)

The alternative to std::iterator is trivial to achieve: just implement the 5 aliases inside of your custom iterators. 

```cpp
class MyIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = int;
    using difference_type   = int;
    using pointer           = int*;
    using reference         = int&;
  
    // ...
}
```
