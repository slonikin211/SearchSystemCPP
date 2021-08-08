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


#include "ReadInputFunctions.hpp"   // Для чтения с потока cin
#include "StringProcessing.hpp"     // Для обработки строк
#include "Document.hpp"             // Класс "Дркумент"




// Максимальное количество документов в результате поиска
const int MAX_RESULT_DOCUMENT_COUNT = 5;


// ================================= Поисковой сервер ================================= //

/*
    SearchServer - класс, предназначенный для поиска актуальных документов по запросам,
    используя ранжирование TF-IDF
    ----------------------------------------------------------------------------------
    Имеет следующий интерфейс:
    1. Конструктор - устанавливает стоп-слова, которые просто игнорируются в документах
    2. AddDocument - добавляет документ в SearchServer, а также его рейтинг
    3. FindTopDocuments - выводит топ документов (до 5) по запросу
    4. GetDocumentCount - возвращает информацию о кол-ве документов
*/
template <typename StringContainer>
class SearchServer {
public:
    // Конструктор, который принимает контейнер со стоп-словами
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
    struct QueryWord {
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
    std::vector<Document> FindAllDocuments(const Query& query, Filter filter) const;
};

// Explicit instantiation
template class SearchServer<std::vector<std::string>>;
template class SearchServer<std::set<std::string>>;
