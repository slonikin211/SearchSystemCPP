#include "search_server.h"

#include "string_processing.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const char* stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}


// ------------------------------- Interaction with the class (public) ------------------------------- //

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
    // Verifing document id
    if (document_id < 0 || documents_extra_.count(document_id)) 
    {
        throw std::invalid_argument("Error! Invalid id of document!");
    }
    if (!IsValidWord(document)) 
    {
        throw std::invalid_argument("Error! Line has invalid symbols!");
    }

    // Now we have stored strings and we can use string_view
    const auto [it, _] = documents_extra_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });

    // Saving document data without stop words
    const auto words = SplitIntoWordsNoStop(it->second.content);

    // Finding the fraction of 1 word in the document 
    const int words_size = words.size();
    const double inv_word_count = 1.0 / words_size;

    // Saving the data about the document in the required format (needed for TF-IDF) 
    // And calculate words frequncies in the document
    for (const auto word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    // Calculate words frequncies in the document
    std::set<std::string_view> unique_words(words.begin(), words.end());
    for (const std::string_view word : unique_words)
    {
        document_to_word_freqs_[document_id].insert({word, 
            std::count(words.begin(), words.end(), word) / static_cast<double>(words_size)});
    }

    // Loging the document
    document_ids_.insert(document_id);
    ++document_count_;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const
{
    return FindTopDocuments(std::execution::seq, raw_query, document_status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
    return FindTopDocuments(std::execution::seq, raw_query);
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

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string_view, double> words_freqs;

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

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    // Получаем список плюс- и минус-слов
    const Query query = ParseQuery(raw_query);

    // Хранилище для значимых слов
    std::vector<std::string_view> matched_words;

    // Пробегаемся по плюс-словам ...
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    // ... и по минус-словам
    for (const std::string_view word : query.minus_words) {
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

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    if (document_ids_.find(document_id) != document_ids_.end())
    {
        for (auto [document, freqs] : document_to_word_freqs_.at(document_id))
        {
            word_to_document_freqs_.erase(document);
        }
        document_to_word_freqs_.erase(document_id);
        documents_extra_.erase(document_id);
        document_ids_.erase(document_id);
        --document_count_;
    }
}

// Parallel version of RemoveDocument(int) with sequenced_policy
void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
    RemoveDocument(document_id);
}

// Parallel version of RemoveDocument(int) with parallel_policy
void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
    RemoveDocument(document_id);
}


// ------------------------------- Private ------------------------------- //

bool SearchServer::IsStopWord(const std::string_view word) const 
{
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
    std::vector<std::string_view> words;
    for (auto word : SplitIntoWords(text))
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

bool SearchServer::IsValidWord(const std::string_view word) 
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), 
        [](char c) 
        {
            return c >= '\0' && c < ' ';
        });
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const 
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

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    return SearchServer::ParseQuery(std::execution::seq, text);
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const 
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}



// ------------------------------- explicit instanciation------------------------------- //

// explicit constructor of string container
// template SearchServer::SearchServer(const std::vector<std::string>&);
// template SearchServer::SearchServer(const std::set<std::string>&);
