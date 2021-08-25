#include "include/RequestQueue.hpp"

// ------------------------------- Constructors ------------------------------- //

 RequestQueue::RequestQueue(const SearchServer& search_server)
    :server_(search_server)
{
}



// ------------------------------- Interface (public) ------------------------------- //

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) 
{
    std::vector<Document> found = server_.FindTopDocuments(raw_query, document_predicate);
    
    QueryResult result = {(found.empty()) ? (true) : (false)};
    requests_.push_back(result);
    if (requests_.size() > sec_in_day_) 
    {
        requests_.pop_front();
    }

    return found;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) 
{
    return AddFindRequest(raw_query, 
        [status](int document_id, DocumentStatus document_status, int rating) 
        {
            return document_status == status;
        });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) 
{
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const 
{
    int noresult = 0;
    for (auto it = requests_.begin(); it != requests_.end(); ++it) 
    {
        if ((*it).isEmpty) 
        {
            ++noresult;
        }
    }
    return noresult;
}


// // explicit instanciation
template std::vector<Document> RequestQueue::AddFindRequest(const std::string&,
    std::function<bool (int, DocumentStatus, int)>);
