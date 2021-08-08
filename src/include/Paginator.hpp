#pragma once

/*
    Пагинатор - класс, использующий итераторы для разделения контейнеры на "страницы"
    Конструктор - итератор begin, итератор end, размер страницы page_size
*/


#include <iostream>
#include <vector>
#include <deque>

#include "SearchServer.hpp"

// Шаблонный класс "страница", типо итератор на начало страницы и на конец страницы
// Последовательность IteratorRange может использоваться в распределении документов по страницам
// при выводе результата запроса на поисковой системе
template <typename Iterator>
class IteratorRange {
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

// Paginator - класс, который хранит страницы, т.е. вектор IteratorRange
template <typename Iterator>
class Paginator {
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