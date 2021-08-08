#pragma once

/*
    RequestQueue - класс "очередь запросов", задача которого делать учет поступающих запросов
    У запросов есть статусы (выполненные/невыполненные)
    Запросы, которые не были выполнены слишком давно отправляются в небытие, уступая место новым
*/

#include <vector>
#include <string>
#include <deque>

#include "SearchServer.hpp"


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool isEmpty;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
};