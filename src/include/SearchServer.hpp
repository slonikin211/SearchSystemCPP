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

    // Конструктор, который принимает строку (string) из стоп слов
    explicit SearchServer(const std::string& stop_words_text);

    // Конструктор, который принимает строку (const char*) из стоп слов
    explicit SearchServer(const char* stop_words_text);

private:
    // Структура для хранения дополнительной информации о документе: рейтинг и статус
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    // Множество стоп-слов
    std::set<std::string> stop_words_;

    // Структура данных, которая хранит информацию о каждом слове:
    // ИД документов где встречается это слово, доля в этих документах
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    // Структура данных для хранения дополнительной информации о документах
    std::map<int, DocumentData> documents_extra_;

    // Кол-во документов
    size_t document_count_ = 0;

    // История добавления документов (нужно для DocumentGetId(int index))
    std::vector<int> document_ids_;

public: 
    
    // Добавить документ
    // Параметры - ИД документа, содержимое документа, статус документа, рейтинги документа
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    // Найти топ документы. Используется шаблон и его специализации
    // Параметры - запрос
    // Дополнительные параметры (специализация) - статус документа | функция-предикат
    template <typename Filter>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Filter filter) const;    
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;

    // Вернуть информацию о значимых словах в документе
    // Необходимо учитывать только плюс-слова, если встретилось
    // хотя бы одно минус-слово - вернуть вектор слов пустым вместе 
    // со статусом документа
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    
private: 
    // Проверяет, является ли слово стоп-словом
    // Параметры - слово
    bool IsStopWord(const std::string& word) const;
    
    // Возвращает содержимое документа без стоп слов
    // Параметры - содержимое документа
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    
    // Посчитать средний рейтинг документа
    // Параметры - вектор рейтингов
    static int ComputeAverageRating(const std::vector<int>& ratings);

    // Проверяет наличие спецсимволов в строке
    static bool IsValidWord(const std::string& word);

    // Структура для хранения информации о слове
    struct QueryWord 
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    
    // Распределить слово в запросе в множества плюс- или минус-слов
    // Параметры - слово запроса
    QueryWord ParseQueryWord(std::string text) const;
    
    // Структура для хранения множеств плюс- и минус-слов для запроса
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    
    // Спарсить строку в запрос
    // Параметры - строка с запросом
    Query ParseQuery(const std::string& text) const;
    
    // Посчитать IDF слова
    // Параметры - значимое слово в документе
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // Найти все документы в SearchServer по запросу. Filter для фильтрация документов (предикат)
    // Параметры - запрос
    template <typename Filter>
    std::vector<Document> FindAllDocuments(const Query& query, Filter filter) const
    {
        // Релевантность документов
        std::map<int, double> document_to_relevance;

        // Считаем релевантность документа используя TF-IDF
        for (const std::string& word : query.plus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }

            // Находим IDF слова ...
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            
            // Фильтруем документы по плюс словам (по слову находим словарь документов, где ключ - ИД документа
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) 
            {
                // Для быстрого доступа к дополнительной информации документа 
                const auto& document_extra_data = documents_extra_.at(document_id);

                // Если документ проходит через фильтр, считаем TF-IDF
                if (filter(document_id, document_extra_data.status, document_extra_data.rating)) 
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        // Удаляем документы с минус-словами из результата,
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

        // Подготавливаем результат для возврата информации о всех документах по запросу, также фильтруем
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

        // Возвращаем все документы по запросу
        return matched_documents;
    }
};
