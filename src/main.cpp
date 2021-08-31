// Park George Sergeevich (slonikin211) - student of Yandex Practicum | C++ Developer
// Start of the project 02.06.2021
// Finish of the project development 02.08.2021
// Tasks:
// Creating Search System with TF-IDF ranging of documents
// Crearing framework for testing (TDD)
// Exceptions
// Using template iterators for Paginator

// After main development
// Template definition in a .cpp file (using explicit instanciation)
// 31.08.2021 - Duration test functionality

#include <iostream>
using namespace std;

#include "include/SearchServer.hpp"
#include "include/TestSearchServer.hpp"
#include "include/Paginator.hpp"
#include "include/RequestQueue.hpp"
#include "include/LogDuration.hpp"

int main() {
    Test_SearchServer::TestSearchServer();

    // If you see this line, all tests are passed
    // cout << "Search server testing finished"s << endl;

    LOG_DURATION("Standart hello");
    LOG_DURATION_STREAM("Hello from stream"s, cout);

    return 0;
}
