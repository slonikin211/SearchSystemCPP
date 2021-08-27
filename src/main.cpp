// Park George Sergeevich (slonikin211) - student of Yandex Practicum | C++ Developer
// Start of the project 02.06.2021
// Finish of the project development 02.08.2021
// Tasks:
// Creating Search System with TF-IDF ranging of documents
// Creating framework for testing (TDD)
// Exceptions
// Using template iterators for Paginator
// Template definition in a .cpp file (using explicit instanciation)


#include <iostream>
using namespace std;

#include "include/SearchServer.hpp"
#include "include/TestSearchServer.hpp"
#include "include/Paginator.hpp"
#include "include/RequestQueue.hpp"

int main() {
     Test_SearchServer::TestSearchServer();

    // If you see this line, all tests are passed
    cout << "Search server testing finished"s << endl;
    return 0;
}
