#pragma once

#include <vector>
#include <string>

#include "SearchServer.hpp"


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        :server(search_server)
    {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> found = server.FindTopDocuments(raw_query, document_predicate);
        
        QueryResult result = {(found.empty()) ? (true) : (false)};
        requests_.push_back(result);

        if (requests_.size() > sec_in_day_) {
            requests_.pop_front();
        }

        return found;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const {
        int noresult = 0;
        for (auto it = requests_.begin(); it != requests_.end(); ++it) {
            if ((*it).isEmpty) {
                ++noresult;
            }
        }
        return noresult;
    }
private:
    struct QueryResult {
        bool isEmpty;
    };
    const SearchServer& server;
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
};