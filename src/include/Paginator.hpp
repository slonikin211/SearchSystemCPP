#pragma once

// Paginator - a class that uses iterators to split containers into "pages" 
// Constructor - begin iterator, end iterator, page_size 

#include <iostream>
#include <vector>
#include <deque>

#include "SearchServer.hpp"

// Template class "page", typically an iterator to the top of the page and to the end of the page
// The IteratorRange sequence can be used in paging documents
// when displaying the result of a query on a search engine 

template <typename Iterator>
class IteratorRange 
{
public:
    IteratorRange(Iterator begin, Iterator end);

    Iterator begin() const;
    Iterator end() const;
    size_t size() const;

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
    Paginator(Iterator begin, Iterator end, size_t page_size);

    auto begin() const;
    auto end() const;
    size_t size() const;

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size);

// Явное инстанцирование
// ......