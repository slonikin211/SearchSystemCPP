#ifndef TEST_SEARCH_SERVER_HPP
#define TEST_SEARCH_SERVER_HPP

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <tuple>

using namespace std;

#include "SearchServer.hpp"     // Класс поисковой системы для тестирования

namespace Test_SearchServer
{
    // Тест-функция для макроса ASSERT
    template <typename T>
    void AssertImpl(const T& t, const string& t_str, const string& file, const string& func,
        unsigned line, const string& hint) 
    {
        if (!t) {
            cerr << file << "("s << line << "): "s << func << ": "s << "ASSERT(" << t_str << ") failed.";
            if (!hint.empty()) {
                cerr << " Hint: " << hint;
            }
            cerr << endl;
            abort();
        }
    }

    #define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
    #define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, hint)



    // Тест-функция для макроса ASSERT_EQUAL
    template <typename T, typename U>
    void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
        const string& func, unsigned line, const string& hint) 
        {
        if (t != u) {
            cerr << boolalpha;
            cerr << file << "("s << line << "): "s << func << ": "s;
            cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
            cout << t << " != "s << u << "."s;
            if (!hint.empty()) {
                cerr << " Hint: "s << hint;
            }
            cerr << endl;
            abort();
        }
    }

    #define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
    #define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))



    // Функция для запуска теста через макрос RUN_TEST
    template <typename TestFunction>
    void RunTestImpl(TestFunction test_func, string test_function_string) {
        test_func();
        std::cerr << test_function_string << " OK" << std::endl;
    }

    #define RUN_TEST(func)  RunTestImpl(func, #func);


    // -------- Начало модульных тестов поисковой системы ----------

    // Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
    void TestExcludeStopWordsFromAddedDocumentContent() {
        const int doc_id = 1;
        const string content = "cat in the city"s;
        const vector<int> ratings = {1, 2, 3};

        // Документы со стоп-словами находятся
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("in"s);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);
        }

        // Документы по запросу, содержищие стоп-слова, не находятся
        {
            SearchServer server;
            server.SetStopWords("in the"s);
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
        }
    }

    // Тест проверяет, что добавленный документ будет находиться по поисковому запросу, который содержит слова документа
    void TestAddDocumentWithQueryWords()
    {
        const int doc_id = 1;
        const string content = "cat in the city"s;
        const vector<int> ratings = {1, 2, 3};

        // Проверяем, что если содержимое документа не пересекается с запросом, то в результате выполнения запроса
        // ничего не находится
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("and"s);
            ASSERT_EQUAL(found_docs.size(), 0u);
        }

        // Проверяем, что добавленный документ, отличный от ACTUAL, не будет находиться, даже
        // если есть пересечения по словам
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
            const auto found_docs = server.FindTopDocuments("cat in the"s);
            ASSERT_EQUAL(found_docs.size(), 0u);
        }

        // Проверяем, что документ, отличный от ACTUAL, будет находиться при условии
        // что запрос содержит статус документа в различных видах
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
            const auto found_docs = server.FindTopDocuments("cat in"s, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);

            // Проверка на предикат-функцию
            const auto found_docs1 = server.FindTopDocuments("cat in"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1;});
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc01 = found_docs[0];
            ASSERT_EQUAL(doc01.id, doc_id);
        }

        // Проверяем, что запрос выполняется по умолчанию
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("cat in"s);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);
        }
    }

    // Тест проверят, что документы с минус-словами не входят в результат поиска
    void TestMinusWordsNotInTheResult()
    {
        const int doc_id_1 = 1, doc_2 = 2;
        const string content1 = "cat in the city"s, content2 = "bee in the boat"s;
        const vector<int> ratings1 = {1, 2, 3}, ratings2 = {-1, -2, -3};
        
        // Проверяем, что документ без минусов слов находится, а с минус-словами - нет
        {
            SearchServer server;
            server.AddDocument(doc_id_1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_2, content2, DocumentStatus::ACTUAL, ratings2);
            const auto found_docs = server.FindTopDocuments("in the -bee"s);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id_1);
        }
    }

    // Тест проверяет матчинг документов
    void TestDocumentMatching()
    {
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const string content1 = "cat in the city"s, content2 = "bee in the boat"s, content3 = "dog in da home"s;
        const vector<int> ratings = {1, 2, 3};
        
        // Проверка на полное соответсвие документов запросу 
        {
            SearchServer server;
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

            const auto found_docs = server.FindTopDocuments("in the -dog"s);
            ASSERT_EQUAL(found_docs.size(), 2u);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            ASSERT_EQUAL(doc0.id, doc_id1);
            ASSERT_EQUAL(doc1.id, doc_id2);
        }
        
        // Проверка на пустой вектор, если документы содержат минус-слова
        {
            SearchServer server;
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

            const auto found_docs = server.FindTopDocuments("-in the -dog"s);
            ASSERT_EQUAL(found_docs.size(), 0u);
        }
    }

    // Тест на сортировку по релевантности
    void TestFoundDocumentSortedByRelevance()
    {
        const string stop_words = "и в на"s;
        const size_t doc_count = 3;
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const string content1 = "белый кот и модный ошейник"s, content2 = "пушистый кот пушистый хвост"s, content3 = "ухоженный пёс выразительные глаза"s;
        const string query = "пушистый ухоженный кот"s;

        // Проверка на сортировку по релевантности документов (заранее посчитано)
        {
            SearchServer server;
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, {3, 3, 3});
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, {4, 4, 4});
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, {5, 5, 5});

            const auto found_docs = server.FindTopDocuments(query);
            ASSERT_EQUAL(found_docs.size(), doc_count);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            const Document& doc2 = found_docs[2];

            ASSERT_EQUAL(doc0.id, doc_id2);
            ASSERT_EQUAL(doc1.id, doc_id3);
            ASSERT_EQUAL(doc2.id, doc_id1);

            ASSERT_EQUAL(doc0.rating, 4);
            ASSERT_EQUAL(doc1.rating, 5);
            ASSERT_EQUAL(doc2.rating, 3);

            ASSERT((doc0.relevance < doc1.relevance) < doc2.relevance);
        }
    }

    // Тест на вычисления рейтинга документа
    void TestRatingOfTheDocument()
    {
        const int doc_id = 1;
        const string content = "cat in the city"s;
        const vector<int> ratings = {1, 2, 3, 4 ,5, 6, 7, 8};

        // Проверка на подсчет среднего рейтинга
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            
            const auto found_documents = server.FindTopDocuments("cat in the"s);
            ASSERT_EQUAL(found_documents.size(), 1u);
            const Document& f_doc = found_documents[0];
            ASSERT_EQUAL(f_doc.rating, 4);
        }
    }

    // Тест на предикат реализован в AddDocument

    // Тест на поиск документов с заданным статусом
    void TestFindDocumentByStatus()
    {
        const string stop_words = "и в на"s;
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const string content1 = "белый кот и модный ошейник"s, content2 = "пушистый кот пушистый хвост"s, content3 = "ухоженный пёс выразительные глаза"s;
        const string query = "пушистый ухоженный кот"s;

        // Проверка на статус по умолчанию
        {
            SearchServer server;
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, {3, 3, 3});
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, {4, 4, 4});
            server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, {5, 5, 5});

            const auto found_docs = server.FindTopDocuments(query);
            ASSERT_EQUAL(found_docs.size(), 2u);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            ASSERT_EQUAL(doc0.id, doc_id2);
            ASSERT_EQUAL(doc1.id, doc_id1);
        }

        // Проверка на статус по заданному напрямую в запросе
        {
            SearchServer server;
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, {3, 3, 3});
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, {4, 4, 4});
            server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, {5, 5, 5});

            const auto found_docs = server.FindTopDocuments(query, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id3);
        }
    }

    // Корректное вычисление релевантности системы
    void TestCorrectRelevanceOfTheSystem()
    {
        const string stop_words = "и в на"s;
        const size_t doc_count = 3;
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const string content1 = "белый кот и модный ошейник"s, content2 = "пушистый кот пушистый хвост"s, content3 = "ухоженный пёс выразительные глаза"s;
        const string query = "пушистый ухоженный кот"s;

        // Проверка на сортировку по релевантности документов (заранее посчитано)
        {
            SearchServer server;
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, {3, 3, 3});
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, {4, 4, 4});
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, {5, 5, 5});

            const auto found_docs = server.FindTopDocuments(query);
            ASSERT_EQUAL(found_docs.size(), doc_count);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            const Document& doc2 = found_docs[2];

            ASSERT_EQUAL(doc0.id, doc_id2);
            ASSERT_EQUAL(doc1.id, doc_id3);
            ASSERT_EQUAL(doc2.id, doc_id1);

            ASSERT_EQUAL(doc2.rating, 4);
            ASSERT_EQUAL(doc2.rating, 5);
            ASSERT_EQUAL(doc2.rating, 3);

            const double EPS = 0.0001;
            ASSERT(abs(doc0.relevance - 0.650672) < EPS);
            ASSERT(abs(doc1.relevance - 0.274653) < EPS);
            ASSERT(abs(doc2.relevance - 0.101366) < EPS);
        }
    }

    // Функция TestSearchServer является точкой входа для запуска тестов
    void TestSearchServer() {
        RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
        RUN_TEST(TestAddDocumentWithQueryWords);
        RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
        RUN_TEST(TestMinusWordsNotInTheResult);
        RUN_TEST(TestDocumentMatching);
        RUN_TEST(TestFoundDocumentSortedByRelevance);
        RUN_TEST(TestRatingOfTheDocument);
        RUN_TEST(TestFindDocumentByStatus);
    }

    // --------- Окончание модульных тестов поисковой системы -----------

}

#endif // !TEST_SEARCH_SERVER_HPP
