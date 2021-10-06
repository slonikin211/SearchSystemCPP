#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <functional>
#include <numeric>
#include <execution>
#include <string_view>

// Maximum amount of documents in the search result
const int MAX_RESULT_DOCUMENT_COUNT = 5;


// ================================= Search system ================================= //

/*
    SearchServer - class using TF-IDF algorithm for ranging documents
    ----------------------------------------------------------------------------------
    Functionality:
    1. Constructor - setting up stop-words which will be ingnored in documents
    2. AddDocument - adding document data and document rating
    3. FindTopDocuments - returning vector of found documents (up to MAX_RESULT_DOCUMENT_COUNT)
    4. GetDocumentCount - returning amount of documents in the SearchServer
*/

class SearchServer 
{
    // Structure for storing additional document data: rating and status
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
        std::string content;
    };

    // Structure for storing information about a word
    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    // Structure for storing sets of plus- and minus-words for a query
    struct Query
    {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

public:
    // Param of constructor is a container of string words
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    {
        for (const auto& str : stop_words) 
        {
            if (!str.empty())  // line is not empty ...
            {
                if (!IsValidWord(str))  // ... and has only valid symbols
                {
                    throw std::invalid_argument("Error! Line has invalid symbols!");
                }
                stop_words_.emplace(str);
            }
        }
    }

    // Constuructor that accepts the std::string with stop-words
    explicit SearchServer(const std::string& stop_words_text);
    // Constuructor that accepts the const char* with stop-words
    explicit SearchServer(const char* stop_words_text);
    // Constuructor that accepts the string_view with stop-words
    explicit SearchServer(const std::string_view stop_words_text);

    
    // Add document
    // Params - id, content, status, rating
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    // Find top documents using template and specializations
    // Params - query
    // Additional params (specialization) - document status | predicate function
    template <typename Filter>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, Filter filter) const
    {
        return FindTopDocuments(std::execution::seq, raw_query, filter);
    }
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <class ExecutionPolicy, typename Filter>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,const std::string_view raw_query, Filter filter) const
    {            
        // Get query with plus- and minus-words
        const Query query = ParseQuery(policy, raw_query);
        
        // Get all documents by predicate
        auto matched_documents = FindAllDocuments(policy, query, filter);
        
        // First of all sort by relevance, then by rating
        auto& documents_for_status = documents_extra_;         
        std::sort(policy,
            matched_documents.begin(), matched_documents.end(), 
            [filter, &documents_for_status](const Document& lhs, const Document& rhs) 
            {
                if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) 
                {
                    return lhs.rating > rhs.rating;
                } 
                else 
                {
                    return lhs.relevance > rhs.relevance;
                }
            });

        // Max document count
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(policy, raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
            return document_status == status;
            });
    }
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    // begin and end for iterating in range-based for
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    // Find word frequrncies in a document by id
    // Return map where key is a word and value is a percentage of the word in the document
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    // Removing document from the server by id
    // Required complexity is o(W * logN) where W is amount of words in a document
    void RemoveDocument(int document_id);

    // Parallel version of RemoveDocument with sequenced_policy
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);

    // Parallel version of RemoveDocument with parallel_policy
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

    // Return information about significant words in the document
    // Only take into account plus-words if met
    // at least one negative word - return a vector of words empty together
    // with document status 
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

    // Parallel version of MatchDocument with sequenced_policy
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const;

    // Parallel version of MatchDocument with parallel_policy
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const;

private: 
    // Checks if a word is a stop-word 
    bool IsStopWord(const std::string_view word) const;
    
    // Returns the content of the document without stop words
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;
    
    // Calculate the average rating of a document
    static int ComputeAverageRating(const std::vector<int>& ratings);

    // Checks for special characters in a string
    static bool IsValidWord(const std::string_view word);

    
    // Distribute a word in a query into sets of plus- or minus-words
    QueryWord ParseQueryWord(std::string_view text) const;
    
    // Parse the line into a query
    Query ParseQuery(const std::string_view text) const;

    template<class ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy&& policy, const std::string_view text) const 
    {
        // Check query:
        // 1. Special symbols
        // 2. More than one minus before minus-words (if minus at the midle it is ok)
        // 3. After minus there is no text

        // 1. Special symbols
        if (!IsValidWord(text))
        {
            throw std::invalid_argument("Error! Line has invalid symbols!");
        }

        // 2. Several minuses in a row
        const size_t size = text.size();
        for (size_t i = 0u; i + 1u < size; ++i)
        {
            if (text[i] == '-')
            {
                // After minus there is minus
                if (text[i + 1u] == '-')
                {
                    throw std::invalid_argument("Several minuses in a row!");
                }
                // 3.1. After minus there is space
                if (text[i + 1u] == ' ')
                {
                    throw std::invalid_argument("No text after minus!");
                }
            }
        }
        // ... And checking boundary variants
        if (text.size() == 1u)
        {
            if (text[0u] == '-')
            {
                throw std::invalid_argument("Here is only minus and nothing else!");
            }
        }
        if (text.size() >= 2u)
        {
            // 3.2. Minus at the end
            if (text[size - 1u] == '-')
            {
                throw std::invalid_argument("No text after minus!");
            }
        }

        Query query;
        for (const std::string_view word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
    
    // Calculate IDF of word
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    // Find all documents in SearchServer by query. Filter for filtering documents (predicate) 
    // Note* : cannot use first template with ExecutionPolicy because of avoiding temp copy between two function calls
    template <typename Filter>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, Filter filter) const
    {
        // Relevance
        std::map<int, double> document_to_relevance;

        // Calculate relevance using TF-IDF
        for (auto word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }

            // Find IDF of word ...
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            // Filter documents by plus words (by word we find a document dictionary, where the key is the document ID 
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                // For quick access to additional document information
                const auto& document_extra_data = documents_extra_.at(document_id);

                // If the document passes through the filter, calculate TF-IDF 
                if (filter(document_id, document_extra_data.status, document_extra_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        // Remove documents with negative keywords from the result
        for (auto word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto document_to_erase : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_to_erase.first);
            }
        }

        // Prepare the result for returning information about all documents upon query, we also filter it
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {
                    document_id,
                    relevance,
                    documents_extra_.at(document_id).rating
                });
        }

        // Return result documents
        return matched_documents;

    }
    template <typename Filter>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, Filter filter) const
    {
        // Relevance
        ConcurrentMap<int, double> document_to_relevance;

        // Calculate relevance using TF-IDF
        std::for_each(
            std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            [&](const auto word)
            {
                if (word_to_document_freqs_.count(word) == 0)
                {
                    return;
                }

                // Find IDF of word ...
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

                // Filter documents by plus words (by word we find a document dictionary, where the key is the document ID 
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
                {
                    // For quick access to additional document information
                    const auto& document_extra_data = documents_extra_.at(document_id);

                    // If the document passes through the filter, calculate TF-IDF 
                    if (filter(document_id, document_extra_data.status, document_extra_data.rating))
                    {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        );


        // Remove documents with negative keywords from the result
        std::for_each(
            std::execution::par,
            query.minus_words.begin(), query.minus_words.end(),
            [&](const auto word)
            {
                if (word_to_document_freqs_.count(word) == 0)
                {
                    return;
                }
                for (const auto [document_id, freq] : word_to_document_freqs_.at(word))
                {
                    document_to_relevance.Erase(document_id);
                }
            }
        );


        // Prepare the result for returning information about all documents upon query, we also filter it
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        {
            matched_documents.push_back(
                {
                    document_id,
                    relevance,
                    documents_extra_.at(document_id).rating
                });
        }

        // Return result documents
        return matched_documents;
    }


private:

    // Set of stop words
    std::set<std::string, std::less<>> stop_words_;

    // Data structure that stores information about each word:
    // ID of documents where this word occurs, share in these documents 
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;

    // Data structure that stores information about each document:
    // Key - id of a document, value - map of words frequencies
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;

    // Data structure for storing additional information about documents 
    std::map<int, DocumentData> documents_extra_;

    // Amount of documents
    size_t document_count_ = 0;

    // History of adding documents
    std::set<int> document_ids_;
};
