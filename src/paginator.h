// Paginator - a class that uses iterators to split containers into "pages" 
// Constructor - begin iterator, end iterator, page_size 
#pragma once


#include <iostream>
#include <vector>
#include <deque>
#include <numeric>

// Template class "page", typically an iterator to the top of the page and to the end of the page
// The IteratorRange sequence can be used in paging documents
// when displaying the result of a query on a search engine 

template <typename Iterator>
class IteratorRange 
{
public:
    IteratorRange(Iterator begin, Iterator end)   
    : first_(begin), last_(end), size_(std::distance(first_, last_))
    {}

    Iterator begin() const
    {
        return first_;
    }
    Iterator end() const
    {
        return last_;
    }
    size_t size() const
    {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range);

// Paginator - the class that stores the pages, i.e. vector IteratorRange
template <typename Iterator>
class Paginator 
{
public:
    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        for (size_t left = std::distance(begin, end); left > 0;) 
        {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = std::next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const
    {
        return pages_.begin();
    }
    auto end() const
    {
        return pages_.end();
    }
    size_t size() const
    {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) 
{
    for (Iterator it = range.begin(); it != range.end(); ++it) 
    {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) 
{
    return Paginator(begin(c), end(c), page_size);
}
