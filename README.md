# Search System C++

C++ search engine that using the TF-IDF ranging algorithm for searching documents by user queries. The query contains words from documents. Negative keywords can also be used, which can lead to the issuance of documents in the search without these keywords. The main feature of the project is the speed of document searching. At the initialization stage, the correct data structure is created for the fastest possible search for documents added to the system. At the main stage, parallel algorithms and multithreading are used.

## Installation

❗ C++17  
❗ tbb library


Use CMakeLists.txt for installation. For example, in root folder:
```bash
mkdir build
cmake ..
```
Depending on your CMake generator, build the project

## Usage

Here is simple demonstation of usage the search system:
```
#include "process_queries.h"
#include "search_server.h"

#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }


    cout << "ACTUAL by default:"s << endl;
    // common version
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // common version
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    // parallel version
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}
```

Also you can use pagination system for getting result by pages:
```
#include "search_server.h"
#include "paginator.h"
#include <iostream>


int main() {
    SearchServer search_server("and in on"s);

    search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "fluffy dog and fashionable collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fashion collar"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog starling evgeniy"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog starling vasily"s, DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("fluffy dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Output found documents by pages
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }

    return 0;
}
```

Also modeled a request queue to remove requests with null result:
```
#include <iostream>

using namespace std;

#include "search_server.h"

int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 queries with no result
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // Still 1439 requests with zero result
    request_queue.AddFindRequest("curly dog"s);

    // new day, first query is deleted, 1438 queries with null result
    request_queue.AddFindRequest("big collar"s);
    
    // first query is deleted, 1437 queries with null result
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}
```