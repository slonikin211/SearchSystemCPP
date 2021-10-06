#include "process_queries.h"
#include <algorithm>
#include <execution>

// Finding result of queries. For every query in queries result is a vector of documents
// Params: SearchServer, vector<string> queries
// Return: vector of vectors of documents
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> res(queries.size());
    std::transform(
        std::execution::par,
        queries.begin(), queries.end(),
        res.begin(),
        [&search_server](const std::string& str){
            return search_server.FindTopDocuments(str);
        });
    return res;
}

// Finding result of queries documents as a "row"
// Params: SearchServer
// Return: vector of documents
std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<Document>res;
    for (const auto& documents : ProcessQueries(search_server, queries))
    {
        for (const auto& document : documents) 
        {
            res.push_back(std::move(document));
        }
    }
    return res;
}