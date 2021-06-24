// Пак Георгий Сергеевич - студент Яндекс Практикума на курсе "Разработчик С++"
// Проект оформлен 25.06.2021

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// Максимальное количество документов в результате поиска
const int MAX_RESULT_DOCUMENT_COUNT = 5;



// ================================= Вспомогательные функции ================================= //

// Считывает строку
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

// Считывает строку с числом
int ReadLineWithNumber() {
    int result;
    cin >> result;  // Вводим число
    ReadLine();     // ...и очищаем буфер для последующего ввода
    return result;
}

// Распредялеят слова в строке в вектор
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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




// ================================= Документ ================================= //
 
// Структура Document - для удобства хранения необходимой информации о документе
struct Document {
    int id;
    double relevance;
    int rating;
};

// Перчисляемый класс DocumentStatus для хранения информации о статусе документа
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
    1. SetStopWords - устанавливает стоп-слов, которые просто игнорируются в документах
    2. AddDocument - добавляет документ в SearchServer, а также его рейтинг
    3. FindTopDocuments - выводит топ документов (до 5) по запросу
    4. GetDocumentCount - возвращает информацию о кол-ве документов в SearchServer
*/

class SearchServer {
private:
    // Структура для хранения дополнительной информации о документе: рейтинг и статус
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    // Множество стоп-слов
    set<string> stop_words_;

    // Структура данных, которая хранит информацию о каждом слове:
    // ИД документов где встречается это слово, релевантность в этих документах
    map<string, map<int, double>> word_to_document_freqs_;

    // Структура данных для хранения дополнительной информации о документах
    map<int, DocumentData> documents_extra;


public:
    // Установить стоп-слова
    // Параметры - строка стоп-слов
    void SetStopWords(const string& text) {
        // Перебериаем слова и сохраняем в множество stop_words_
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }    
    
    // Добавить документ
    // Параметры - ИД документа, содержимое документа, статус документа, рейтинги документа
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        // Сохраняем содержимое документа без стоп слов
        const vector<string> words = SplitIntoWordsNoStop(document);

        // Находим долю 1 слова в документа
        const double inv_word_count = 1.0 / words.size();

        // Сохраняем данные о документе в нужном формате (нужно для TF-IDF)
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        // Сохраняем дополнительную информацию о документе
        documents_extra.emplace(document_id, 
            DocumentData{
                ComputeAverageRating(ratings), 
                status
            });
    }

    // Найти топ документы. Используется шаблон и его специализации
    // Параметры - запрос
    // Дополнительные парамтетры - статус документа | функция-предикат
	template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {            
        // Получаем запрос с плюс- и минус-словами
		const Query query = ParseQuery(raw_query);

		// Получаем все документы, в matched_documents будем сохранять отфильтрованные
        auto all_documents = FindAllDocuments(query);
        vector<Document> matched_documents;

		// Фильтруем документы
		for (const auto& document : all_documents)
			if (filter(document.id, documents_extra.at(document.id).status, document.rating))
				matched_documents.push_back(document);
		
		// Сортируем сначала по релевантности, после по рейтингу
		auto& documents_for_status = documents_extra;         
        sort(matched_documents.begin(), matched_documents.end(),
            [filter, &documents_for_status](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
             });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
		return FindTopDocuments(raw_query, [document_status](int document_id, DocumentStatus status, int rating){ return status == document_status; });
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

    // Вернуть количество документов
    int GetDocumentCount() const {
        return documents_extra.size();
    }


private: 

    // Вернуть информацию о значимых словах в документе
    // Необходимо учитывать только плюс-слова, если встретилось
    // хотя бы одно минус-слово - вернуть вектор слов пустым вместе 
    // со статусом документа
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        // Получаем список плюс- и минус-слов
        const Query query = ParseQuery(raw_query);

        // Хранилище для значимых слов
        vector<string> matched_words;

        // Пробегаемся по плюс-словам ...
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }

        // ... и по минус-словам
        for (const string& word : query.minus_words) {
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
        return {matched_words, documents_extra.at(document_id).status};
    }
    
    // Проверяет, является ли слово стоп-словом
    // Параметры - слово
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    // Возвращает вектор содержимое документа без стоп слов
    // Параметры - содержимое документа
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    // Посчитать средний рейтинг документа
    // Параметры - вектор рейтингов
    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    // Структура для хранения информации о плюс- и минус-словах
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    // Распределить слово в запросе в множества плюс- или минус-слов
    // Параметры - вектор рейтингов
    QueryWord ParseQueryWord(string text) const {
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
    
    // Структура для хранения плюс- и минус слов для запроса
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    // Спарсить строку в запрос
    // Параметры - строка с запросом
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // Найти все документы в SearchServer по запросу
    // Параметры - запрос
    vector<Document> FindAllDocuments(const Query& query) const {
        // Релевантность документов
        map<int, double> document_to_relevance;

        // Считаем релевантность документа используя TF-IDF
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            // Находим IDF слова ...
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            // ... и считаем TF-IDF
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
        
        // Удаляем документы с минус-словами из результата
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto document_to_erase : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_to_erase.first);
            }
        }

        // Подготавливаем результат для возврата информации о всех документах
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_extra.at(document_id).rating
            });
        }

        // Возвращаем все документы по запросу
        return matched_documents;
    }
};




// ================================= Далее пример использования ================================= //

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main(int argc, char* argv[]) {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
} 