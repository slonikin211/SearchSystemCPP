// Park George Sergeevich (slonikin211) - student of Yandex Practicum | C++ Developer
// Start of the project 02.06.2021
// Finish of the project development 02.08.2021
// Tasks:
// Creating Search System with TF-IDF ranging of documents
// Creating framework for testing (TDD)
// Exceptions
// Using template iterators for Paginator

// After main development
// Template definition in a .cpp file (using explicit instanciation)
// 31.08.2021 - Duration test functionality
// 10.09.2021 - Refactoring and class updated (begin and end instead of GetDocumentById, GetWordFrequencies, RemoveDocument, RemoveDuplicates)
// 07.10.2021 - Optimizee using parallel algorythms and threads (mutex)


#include <iostream>
using namespace std;

#include "document.h"
#include "log_duration.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"


#include "process_queries.h"
#include "search_server.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

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
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }


    cout << "ACTUAL by default:"s << endl;
    // ���������������� ������
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // ���������������� ������
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    // ������������ ������
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}