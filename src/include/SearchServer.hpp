#pragma once

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

#include "ReadInputFunctions.hpp"
#include "StringProcessing.hpp"
#include "Document.hpp"


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
public:
    // Param of constructor is a container of string words
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    // Constuructor that accepts the std::string with stop-words
    explicit SearchServer(const std::string& stop_words_text);

    // Constuructor that accepts the const char* with stop-words
    explicit SearchServer(const char* stop_words_text);

private:
    // Structure for storing additional document data: rating and status
    struct DocumentData 
    {
        int rating;
        DocumentStatus status;
    };

    // Set of stop words
    std::set<std::string> stop_words_;

    // Data structure that stores information about each word:
    // ID of documents where this word occurs, share in these documents 
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    // Data structure that stores information about each document:
    // Key - id of a document, value - map of words frequencies
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;

    // Data structure for storing additional information about documents 
    std::map<int, DocumentData> documents_extra_;

    // Amount of documents
    size_t document_count_ = 0;

    // History of adding documents
    std::set<int> document_ids_;

public: 
    
    // Add document
    // Params - id, content, status, rating
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    // Find top documents using template and specializations
    // Params - query
    // Additional params (specialization) - document status | predicate function
    template <typename Filter>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Filter filter) const
    {            
        // Get query with plus- and minus-words
        const Query query = ParseQuery(raw_query);
        
        // Get all documents by predicate
        auto matched_documents = FindAllDocuments(query, filter);
        
        // First of all sort by relevance, then by rating
        auto& documents_for_status = documents_extra_;         
        sort(matched_documents.begin(), matched_documents.end(), 
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
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;

    // begin and end for iterating in range-based for
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    // Find word frequrncies in a document by id
    // Return map where key is a word and value is a percentage of the word in the document
    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    // Removing document from the server by id
    // Required complexity is o(W * logN) where W is amount of words in a document
    void RemoveDocument(int document_id);

    // Return information about significant words in the document
    // Only take into account plus-words if met
    // at least one negative word - return a vector of words empty together
    // with document status 
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    
private: 
    // Checks if a word is a stop-word 
    bool IsStopWord(const std::string& word) const;
    
    // Returns the content of the document without stop words
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    
    // Calculate the average rating of a document
    static int ComputeAverageRating(const std::vector<int>& ratings);

    // Checks for special characters in a string
    static bool IsValidWord(const std::string& word);

    // Structure for storing information about a word
    struct QueryWord 
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    
    // Distribute a word in a query into sets of plus- or minus-words
    QueryWord ParseQueryWord(std::string text) const;
    
    // A structure for storing sets of plus- and minus-words for a query
    struct Query 
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    
    // Parse the line into a query
    Query ParseQuery(const std::string& text) const;
    
    // Calculate IDF of word
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // Find all documents in SearchServer by query. Filter for filtering documents (predicate) 
    template <typename Filter>
    std::vector<Document> FindAllDocuments(const Query& query, Filter filter) const
    {
        // Relevance
        std::map<int, double> document_to_relevance;

        // Calculate relevance using TF-IDF
        for (const std::string& word : query.plus_words) 
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
        for (const std::string& word : query.minus_words) 
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
};
