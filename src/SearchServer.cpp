#include "include/SearchServer.hpp"

#include "include/ReadInputFunctions.hpp"
#include "include/StringProcessing.hpp"
#include "include/Document.hpp"

// ------------------------------- Constructors ------------------------------- //

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    for (const std::string& str : stop_words) {
        if (!str.empty())  // line is not empty ...
        {
            if (IsValidWord(str))  // ... and has only valid symbols
            {
                stop_words_.insert(str);
            }
            else 
            {
                throw std::invalid_argument("В строке содержатся специальные символы");
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
    if (document_id < 0 || documents_extra_.count(document_id)) {
        throw std::invalid_argument("Некорректный id документа");
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Документ содержит специальные символы");
    }

    // Saving document data without stop words
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    // Finding the fraction of 1 word in the document 
    const double inv_word_count = 1.0 / words.size();

    // Saving the data about the document in the required format (needed for TF-IDF) 
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    // Saving advanced data of the document
    documents_extra_.emplace(document_id, 
        DocumentData{
            ComputeAverageRating(ratings), 
            status
        });

    // Loging the document
    document_ids_.push_back(document_id);
    ++document_count_;
}

template <typename Filter>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Filter filter) const 
{            
    // Получаем запрос с плюс- и минус-словами
    const Query query = ParseQuery(raw_query);
    
    // Получаем все документы по запросу и предикату
    auto matched_documents = FindAllDocuments(query, filter);
    
    // Сортируем сначала по релевантности, после по рейтингу
    auto& documents_for_status = documents_extra_;         
    sort(matched_documents.begin(), matched_documents.end(), [filter, &documents_for_status](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) 
        {
            return lhs.rating > rhs.rating;
        } 
        else 
        {
            return lhs.relevance > rhs.relevance;
        }
    });

    // Не забываем про ограничение выводимых документов в топе
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const {
    return FindTopDocuments(raw_query, 
        [document_status](int document_id, DocumentStatus status, int rating) 
        { 
            return status == document_status; 
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return document_count_;
}

int SearchServer::GetDocumentId(int index) const {
    // Выброситься out_of_range, если индекс плохой
    return document_ids_.at(index);
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



// ------------------------------- Private ------------------------------- //

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const 
{
    bool is_minus = false;
    if (text[0] == '-') 
    {
        is_minus = true;
        text = text.substr(1);  // для удаления '-'  в начале
    }
    return 
    {
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    // Проверяем запрос на:
    // 1. Спец. символы
    // 2. Более 1 минуса перед минус-словом (если минус в середине слово, то все ок)
    // 3. После минуса нет текста

    // 1. Спец символы
    if (!IsValidWord(text)) {
        throw std::invalid_argument("В строке содержатся специальные символы");
    }

    // 2. Несколько минусов подряд
    const size_t size = text.size();
    for (size_t i = 0u; i + 1u < size; ++i) {// Берем не всю строку, а до -1 в конце, чтобы можно было обращаться к следующему элементу ...
        if (text[i] == '-') {
            // После минуса есть минус
            if (text [i + 1u] == '-') {
                throw std::invalid_argument("Несколько минусов подряд");
            }
            // 3.1. После минуса пробел
            if (text[i + 1u] == ' ') {
                throw std::invalid_argument("После минуса пусто");
            }
        }
    }
    // ... Ну и проверяем граничные случаи отдельно
    if (text.size() == 1u) {
        if (text[0u] == '-') {
            throw std::invalid_argument("В запросе только минус без ничего");
        }
    }
    if (text.size() >= 2u) {
        // 3.2. В конце минус
        if (text[size - 1u] == '-') {
            throw std::invalid_argument("После минуса пусто");
        }
    }

    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
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

// template <typename Filter>
// std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query, Filter filter) const 
// {
//     // Релевантность документов
//     std::map<int, double> document_to_relevance;

//     // Считаем релевантность документа используя TF-IDF
//     for (const std::string& word : query.plus_words) 
//     {
//         if (word_to_document_freqs_.count(word) == 0) 
//         {
//             continue;
//         }

//         // Находим IDF слова ...
//         const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        
//         // Фильтруем документы по плюс словам (по слову находим словарь документов, где ключ - ИД документа
//         for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) 
//         {
//             // Для быстрого доступа к дополнительной информации документа 
//             const auto& document_extra_data = documents_extra_.at(document_id);

//             // Если документ проходит через фильтр, считаем TF-IDF
//             if (filter(document_id, document_extra_data.status, document_extra_data.rating)) 
//             {
//                 document_to_relevance[document_id] += term_freq * inverse_document_freq;
//             }
//         }
//     }
    
//     // Удаляем документы с минус-словами из результата,
//     for (const std::string& word : query.minus_words) 
//     {
//         if (word_to_document_freqs_.count(word) == 0) 
//         {
//             continue;
//         }
//         for (const auto document_to_erase : word_to_document_freqs_.at(word)) 
//         {
//             document_to_relevance.erase(document_to_erase.first);
//         }
//     }

//     // Подготавливаем результат для возврата информации о всех документах по запросу, также фильтруем
//     std::vector<Document> matched_documents;
//     for (const auto [document_id, relevance] : document_to_relevance) 
//     {
//         matched_documents.push_back(
//             {
//                 document_id,
//                 relevance,
//                 documents_extra_.at(document_id).rating
//             });
//     }

//     // Возвращаем все документы по запросу
//     return matched_documents;
// }

// ------------------------------- explicit instanciation------------------------------- //

// explicit constructor of string container
template SearchServer::SearchServer(const std::vector<std::string>&);
template SearchServer::SearchServer(const std::set<std::string>&);

template std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, 
    std::function<bool (int, DocumentStatus, int)>) const;