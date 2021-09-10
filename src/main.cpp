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

#include <iostream>
using namespace std;

#include "include/SearchServer.hpp"
#include "include/TestSearchServer.hpp"
#include "include/Paginator.hpp"
#include "include/RequestQueue.hpp"
#include "include/LogDuration.hpp"
#include "include/RemoveDuplicates.hpp"

int main() {
    Test_SearchServer::TestSearchServer();
    // If you see this line, all tests are passed
    cout << "Search server testing finished"s << endl;

    return 0;
}
