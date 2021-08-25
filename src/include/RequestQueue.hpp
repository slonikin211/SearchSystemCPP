#pragma once

// RequestQueue - class, whose task is to keep track of incoming requests
// Requests have statuses (completed / unfulfilled)
// Requests that have not been fulfilled for too long are sent into oblivion (lol), giving way to new ones 

#include <vector>
#include <string>
#include <deque>

#include "SearchServer.hpp"


class RequestQueue 
{
public:
    explicit RequestQueue(const SearchServer& search_server);

    // Lets make "wrappers" for all search methods to save results for our statistics 
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult 
    {
        bool isEmpty;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
};