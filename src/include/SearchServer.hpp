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
//#include "StringProcessing.hpp"     // Для обработки строк

/*
    Примечание:
    При нормальном подключении #include "StringProcessing.hpp" возникает ошибка линковки undefined reference to
    `SplitIntoWords(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)'
    в местах вызова этой функции. Погуглил. Вот ссылки на то что пытался сделать, но ничего не помогло:
    https://stackoverflow.com/questions/33394934/converting-std-cxx11string-to-stdstring
    https://techoverflow.net/2021/04/11/how-to-fix-gcc-lots-of-undefined-reference-to-std-functions/
    https://coderoad.ru/39539752/Использование-G-неопределенная-ссылка-std-Makefile
    https://github.com/preshing/junction/issues/37
    https://overcoder.net/q/866515/неопределенная-ссылка-на-процесс-std-cxx11-basicstring-при-компиляции-примеров
    https://root-forum.cern.ch/t/problems-compiling-root-with-gcc-9-1/34423
    
    Насколько я понял, проблема заключается в том что, строки (обычные) не состыковаваются со строками std::string
    при линковке из-за последней версии GCC. Чтобы решить проблему нужно использовать GCC 4.8 или более раннюю, но 
    там нет функционала из C++17, который здесь активно используется, да переустанавливать компилятор на раннюю версию
    не очень хочется. Застрял в тупике, не знаю как это правильно оформить

    Временно насильно определил функцию SplitIntoWords в том же файле, где и вызывается
    Надеюсь Вы мне поможете решить проблему
*/

// Распредялеят слова в строке в вектор
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);
    
    return words;
}


// Максимальное количество документов в результате поиска
const int MAX_RESULT_DOCUMENT_COUNT = 5;


// ================================= Документ ================================= //
 
// Структура Document - для удобства хранения необходимой информации о документе
struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;

    // Конструкторы для инициализации документа
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }
};

// Перечисляемый класс DocumentStatus для хранения информации о статусе документа
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};



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

class SearchServer {
public:
    // Конструктор, который принимает контейнер со стоп-словами
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) {
        for (const std::string& str : stop_words) {
            if (!str.empty()) {     // строка должна быть непустой ...
                if (IsValidWord(str)) {     // ... и не должна содержать спец символы
                    stop_words_.insert(str);
                }
                else {
                    throw std::invalid_argument("В строке содержатся специальные символы");
                }
            }
        }
    }

    // Конструктор, который принимает строку (string) из стоп слов
    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    // Конструктор, который принимает строку (const char*) из стоп слов
    explicit SearchServer(const char* stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

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
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
        // Проверка id документа (неотрицательный и нет документа с таким же id)
        if (document_id < 0 || documents_extra_.count(document_id)) {
            throw std::invalid_argument("Некорректный id документа");
        }
        if (!IsValidWord(document)) {
            throw std::invalid_argument("Документ содержит специальные символы");
        }

        // Сохраняем содержимое документа без стоп-слов
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);

        // Находим долю 1 слова в документе
        const double inv_word_count = 1.0 / words.size();

        // Сохраняем данные о документе в нужном формате (нужно для TF-IDF)
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        // Сохраняем дополнительную информацию о документе
        documents_extra_.emplace(document_id, 
            DocumentData{
                ComputeAverageRating(ratings), 
                status
            });
    
        // Сохраняем документ в историю документов
        document_ids_.push_back(document_id);
        ++document_count_;
    }

    // Найти топ документы. Используется шаблон и его специализации
    // Параметры - запрос
    // Дополнительные параметры (специализация) - статус документа | функция-предикат
    template <typename Filter>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Filter filter) const {            
        // Получаем запрос с плюс- и минус-словами
        const Query query = ParseQuery(raw_query);
        
        // Получаем все документы по запросу и предикату
        auto matched_documents = FindAllDocuments(query, filter);
		
		// Сортируем сначала по релевантности, после по рейтингу
        auto& documents_for_status = documents_extra_;         
        sort(matched_documents.begin(), matched_documents.end(), [filter, &documents_for_status](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });

        // Не забываем про ограничение выводимых документов в топе
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating) { 
            return status == document_status; 
    	});
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    // Вернуть количество документов
    int GetDocumentCount() const {
        return document_count_;
    }

    int GetDocumentId(int index) const {
        // Выброситься out_of_range, если индекс плохой
        return document_ids_.at(index);
    }

    // Вернуть информацию о значимых словах в документе
    // Необходимо учитывать только плюс-слова, если встретилось
    // хотя бы одно минус-слово - вернуть вектор слов пустым вместе 
    // со статусом документа
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const {
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
    
private: 
    // Проверяет, является ли слово стоп-словом
    // Параметры - слово
    bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    // Возвращает содержимое документа без стоп слов
    // Параметры - содержимое документа
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    // Посчитать средний рейтинг документа
    // Параметры - вектор рейтингов
    static int ComputeAverageRating(const std::vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    // Проверяет наличие спецсимволов в строке
    static bool IsValidWord(const std::string& word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    // Структура для хранения информации о слове
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    
    // Распределить слово в запросе в множества плюс- или минус-слов
    // Параметры - слово запроса
    QueryWord ParseQueryWord(std::string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);  // для удаления '-'  в начале
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }
    
    // Структура для хранения множеств плюс- и минус-слов для запроса
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    
    // Спарсить строку в запрос
    // Параметры - строка с запросом
    Query ParseQuery(const std::string& text) const {
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
    
    // Посчитать IDF слова
    // Параметры - значимое слово в документе
    double ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // Найти все документы в SearchServer по запросу. Filter для фильтрация документов (предикат)
    // Параметры - запрос
    template <typename Filter>
    std::vector<Document> FindAllDocuments(const Query& query, Filter filter) const {
        // Релевантность документов
        std::map<int, double> document_to_relevance;

        // Считаем релевантность документа используя TF-IDF
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            // Находим IDF слова ...
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            
            // Фильтруем документы по плюс словам (по слову находим словарь документов, где ключ - ИЛ документа
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                // Для быстрого доступа к дополнительной информации документа 
                const auto& document_extra_data = documents_extra_.at(document_id);

                // Если документ проходит через фильтр, считаем TF-IDF
                if (filter(document_id, document_extra_data.status, document_extra_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        // Удаляем документы с минус-словами из результата,
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto document_to_erase : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_to_erase.first);
            }
        }

        // Подготавливаем результат для возврата информации о всех документах по запросу, также фильтруем
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_extra_.at(document_id).rating
            });
        }

        // Возвращаем все документы по запросу
        return matched_documents;
    }
};

#include "Paginator.hpp"            // Для "страниц" в результатах запроса
#include "RequestQueue.hpp"         // Очередь запросов