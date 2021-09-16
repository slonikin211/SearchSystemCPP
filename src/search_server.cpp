#include "search_server.h"

#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"

// ------------------------------- Constructors ------------------------------- //

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) 
{
    for (const std::string& str : stop_words) 
    {
        if (!str.empty())  // line is not empty ...
        {
            if (IsValidWord(str))  // ... and has only valid symbols
            {
                stop_words_.insert(str);
            }
            else 
            {
                throw std::invalid_argument("Error! Line has invalid symbols!");
            }
        }
    }
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const char* stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}


// ------------------------------- Interaction with the class (public) ------------------------------- //

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) 
{
    // Verifing document id
    if (document_id < 0 || documents_extra_.count(document_id)) 
    {
        throw std::invalid_argument("Error! Not valid id of document!");
    }
    if (!IsValidWord(document)) 
    {
        throw std::invalid_argument("Error! Line has invalid symbols!");
    }

    // Saving document data without stop words
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    // Finding the fraction of 1 word in the document 
    const int words_size = words.size();
    const double inv_word_count = 1.0 /words_size;

    // Saving the data about the document in the required format (needed for TF-IDF) 
    for (const std::string& word : words) 
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    // Calculate words frequncies in the document
    std::set<std::string> unique_words(words.begin(), words.end());
    for (const std::string& word : unique_words) 
    {
        document_to_word_freqs_[document_id].insert({word, 
            std::count(words.begin(), words.end(), word) / static_cast<double>(words_size)});
    }

    // Saving advanced data of the document
    documents_extra_.emplace(document_id, 
        DocumentData{
            ComputeAverageRating(ratings), 
            status
        });

    // Loging the document
    document_ids_.insert(document_id);
    ++document_count_;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const 
{
    return FindTopDocuments(raw_query, 
        [document_status](int document_id, DocumentStatus status, int rating) 
        { 
            return status == document_status; 
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const 
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const 
{
    return document_count_;
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string, double> words_freqs;

    if (!words_freqs.empty())   // because of static variable
    {
        words_freqs.clear();
    }
    
    // Verifing document id
    if (document_id < 0 || !documents_extra_.count(document_id)) 
    {
        return words_freqs;
    }

    words_freqs = document_to_word_freqs_.at(document_id);
    return words_freqs;
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    // Получаем список плюс- и минус-слов
    const Query query = ParseQuery(raw_query);

    // Хранилище для значимых слов
    std::vector<std::string> matched_words;

    // Пробегаемся по плюс-словам ...
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    // ... и по минус-словам
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        // Если минус-слово, чистим вектор слов и выходим из цикла
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    // Возвращаем результат
    return {matched_words, documents_extra_.at(document_id).status};
}

void SearchServer::RemoveDocument(int document_id)
{
    if (document_ids_.find(document_id) != document_ids_.end()) 
    {
        // Can't use structured bindings because of -Werror=unused-value for second parameter
        for (auto word : document_to_word_freqs_.at(document_id)) 
        {
            word_to_document_freqs_.erase(word.first);
        }
        document_to_word_freqs_.erase(document_id);
        documents_extra_.erase(document_id);
        document_ids_.erase(document_id);
        --document_count_;
    }
}

// ------------------------------- Private ------------------------------- //

bool SearchServer::IsStopWord(const std::string& word) const 
{
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const 
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) 
    {
        if (!IsStopWord(word)) 
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) 
{
    int rating_sum = 0;
    for (const int rating : ratings) 
    {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

bool SearchServer::IsValidWord(const std::string& word) 
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), 
        [](char c) 
        {
            return c >= '\0' && c < ' ';
        });
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const 
{
    bool is_minus = false;
    if (text[0] == '-') 
    {
        is_minus = true;
        text = text.substr(1);  // for deleting '-' at the begining
    }
    return 
    {
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
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
            if (text [i + 1u] == '-') 
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
    for (const std::string& word : SplitIntoWords(text)) 
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const 
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}



// ------------------------------- explicit instanciation------------------------------- //

// explicit constructor of string container
template SearchServer::SearchServer(const std::vector<std::string>&);
template SearchServer::SearchServer(const std::set<std::string>&);
