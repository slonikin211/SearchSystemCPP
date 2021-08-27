# Search System C++

Search engine on C++. The TF-IDF ranging algorithm is used to search documents by query. The query can contain words from the searched documents, as well as negative keywords that will ignore documents with these words. To do this, you need to put '-' before the word in the query.

---

## Documentation

### SearchServer

+ Constructor with parameter (stop-words) only. It can accept a string container, std::string text with words and the same with const char*
+ AddDocument method for adding document to the search engine. Params: id, content, status, ratings
+ FindTopDocuments method for finding top documents by a query. Params: query, status (if needed), predicate (if needed)
+ MatchDocument method that returns information about significant words in the document. Params: query, id


### Paginator

+ Constructor with 3 parameters only. Params: begin it, end it, page size
+ begin method that returns begin page iterator. No params
+ end method that returns end page iterator. No params
+ size method that returns amount of pages


### RequestsQueue

+ AddFindRequest method is used for adding query for logging. Params: query, status (if needed), predicate (if needed)
+ GetNoResultRequests method returns amount of queries with empty result

## Examples

### SearchServer class


    #include <iostream>
    #include "include/SearchServer.hpp"

    using namespace std;
    
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


### Paginator

    #include "include/SearchServer.hpp"
    #include "include/Paginator.hpp"
    #include <iostream>


    int main() {
        SearchServer search_server("и в на"s);

        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});

        const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
        int page_size = 2;
        const auto pages = Paginate(search_results, page_size);

        // Выводим найденные документы по страницам
        for (auto page = pages.begin(); page != pages.end(); ++page) {
            cout << *page << endl;
            cout << "Разрыв страницы"s << endl;
        }

        return 0;
    }


### RequestsQueue

    #include <iostream>

    using namespace std;

    #include "include/SearchServer.hpp"         // Класс SearchServer из прошлого спринта
    #include "include/TestSearchServer.hpp"     // Тестирование SearchServer

    int main() {
        SearchServer search_server("and in at"s);
        RequestQueue request_queue(search_server);

        search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

        // 1439 запросов с нулевым результатом
        for (int i = 0; i < 1439; ++i) {
            request_queue.AddFindRequest("empty request"s);
        }
        // все еще 1439 запросов с нулевым результатом
        request_queue.AddFindRequest("curly dog"s);

        // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
        request_queue.AddFindRequest("big collar"s);
        
        // первый запрос удален, 1437 запросов с нулевым результатом
        request_queue.AddFindRequest("sparrow"s);
        cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
        return 0;
    }
