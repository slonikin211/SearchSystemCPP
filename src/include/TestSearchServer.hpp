#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <tuple>

#include "SearchServer.hpp"     // SearchServer class for testing

namespace Test_SearchServer
{
    // Comparing two numbers (double)
    bool fequal(const double num1, const double num2)
    {
        return std::abs(num1 - num2) < 0.0001;  // ... using epsilon
    }

    // Test function for macros ASSERT
    template <typename T>
    void AssertImpl(const T& t, const std::string& t_str, const std::string& file, const std::string& func,
        unsigned line, const std::string& hint) 
    {
        if (!t) {
            std::cerr << file << "(" << line << "): " << func << ": " << "ASSERT(" << t_str << ") failed.";
            if (!hint.empty()) {
                std::cerr << " Hint: " << hint;
            }
            std::cerr << std::endl;
            abort();
        }
    }

    #define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")
    #define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)


    // Test function for macros ASSERT_EQUAL
    template <typename T, typename U>
    void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
        const std::string& func, unsigned line, const std::string& hint) 
        {
        if (t != u) {
            std::cerr << std::boolalpha;
            std::cerr << file << "(" << line << "): " << func << ": ";
            std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
            std::cerr << t << " != " << u << ".";
            if (!hint.empty()) {
                std::cerr << " Hint: " << hint;
            }
            std::cerr << std::endl;
            abort();
        }
    }

    #define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")
    #define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


    // Function for launching tests RUN_TEST
    template <typename TestFunction>
    void RunTestImpl(TestFunction test_func, std::string test_function_string) {
        test_func();
        std::cout << test_function_string << " OK" << std::endl;
    }

    #define RUN_TEST(func)  RunTestImpl(func, #func);


    // -------- The begin of module tests for SearchServer ----------

    // The test verifies that the search engine excludes stop words when adding documents 
    void TestExcludeStopWordsFromAddedDocumentContent() {
        const int doc_id = 1;
        const std::string content = "cat in the city";
        const std::vector<int> ratings = {1, 2, 3};

        // Документы со стоп-словами находятся
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("in");
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);
        }

        // Документы по запросу, содержищие стоп-слова, не находятся
        {
            SearchServer server("in the");
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            ASSERT_HINT(server.FindTopDocuments("in").empty(), "Stop words must be excluded from documents");
        }
    }

    // The test verifies that the added document will be found by a search query that contains the words of the document 
    void TestAddDocumentWithQueryWords()
    {
        const int doc_id = 1;
        const std::string content = "cat in the city";
        const std::vector<int> ratings = {1, 2, 3};

        // Проверяем, что если содержимое документа не пересекается с запросом, то в результате выполнения запроса
        // ничего не находится
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("and");
            ASSERT_EQUAL(found_docs.size(), 0u);
        }

        // Проверяем, что добавленный документ, отличный от ACTUAL, не будет находиться, даже
        // если есть пересечения по словам
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
            const auto found_docs = server.FindTopDocuments("cat in the");
            ASSERT_EQUAL(found_docs.size(), 0u);
        }

        // Проверяем, что документ, отличный от ACTUAL, будет находиться при условии
        // что запрос содержит статус документа в различных видах
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
            const auto found_docs = server.FindTopDocuments("cat in", DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);

            // Проверка на предикат-функцию
            const auto found_docs1 = server.FindTopDocuments("cat in", [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1;});
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc01 = found_docs[0];
            ASSERT_EQUAL(doc01.id, doc_id);
        }

        // Проверяем, что запрос выполняется по умолчанию
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("cat in");
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id);
        }
    }

    // The test verifies that documents with negative keywords are not included in the search result
    void TestMinusWordsNotInTheResult()
    {
        const int doc_id_1 = 1, doc_2 = 2;
        const std::string content1 = "cat in the city", content2 = "bee in the boat";
        const std::vector<int> ratings1 = {1, 2, 3}, ratings2 = {-1, -2, -3};
        
        // Проверяем, что документ без минус слов находится, а с минус-словами - нет
        {
            SearchServer server("");
            server.AddDocument(doc_id_1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_2, content2, DocumentStatus::ACTUAL, ratings2);
            const auto found_docs = server.FindTopDocuments("in the -bee");
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id_1);
        }
    }

    // The test verifies documents matching
    void TestDocumentMatching()
    {
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const std::string content1 = "cat in the city", content2 = "bee in the boat", content3 = "dog in da home";
        const std::vector<int> ratings = {1, 2, 3};
        
        // Проверка на полное соответсвие документов запросу 
        {
            SearchServer server("");
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

            const auto found_docs = server.FindTopDocuments("in the -dog");
            ASSERT_EQUAL(found_docs.size(), 2u);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            ASSERT_EQUAL(doc0.id, doc_id1);
            ASSERT_EQUAL(doc1.id, doc_id2);
        }
        
        // Проверка на пустой вектор, если документы содержат минус-слова
        {
            SearchServer server("");
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

            const auto found_docs = server.FindTopDocuments("-in the -dog");
            ASSERT_EQUAL(found_docs.size(), 0u);
        }
    }

    // The test verifies sorted result
    void TestFoundDocumentSortedByRelevance()
    {
        const std::string stop_words = "и в на";
        const size_t doc_count = 3;
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const std::string content1 = "белый кот и модный ошейник", content2 = "пушистый кот пушистый хвост", content3 = "ухоженный пёс выразительные глаза";
        const std::string query = "пушистый ухоженный кот";

        // Проверка на сортировку по релевантности документов (заранее посчитано)
        {
            SearchServer server("");
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

    // The test verifies rating calculation for documents
    void TestRatingOfTheDocument()
    {
        const int doc_id = 1;
        const std::string content = "cat in the city";
        const std::vector<int> ratings = {1, 2, 3, 4 ,5, 6, 7, 8};

        // Проверка на подсчет среднего рейтинга
        {
            SearchServer server("");
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            
            const auto found_documents = server.FindTopDocuments("cat in the");
            ASSERT_EQUAL(found_documents.size(), 1u);
            const Document& f_doc = found_documents[0];
            ASSERT_EQUAL(f_doc.rating, 4);
        }
    }

    // The test with predicate is realised in TestExcludeStopWordsFromAddedDocumentContent test

    // The test verifies status of result documents
    void TestFindDocumentByStatus()
    {
        const std::string stop_words = "и в на";
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const std::string content1 = "белый кот и модный ошейник", content2 = "пушистый кот пушистый хвост", content3 = "ухоженный пёс выразительные глаза";
        const std::string query = "пушистый ухоженный кот";

        // Проверка на статус по умолчанию
        {
            SearchServer server("");
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
            SearchServer server("");
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, {3, 3, 3});
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, {4, 4, 4});
            server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, {5, 5, 5});

            const auto found_docs = server.FindTopDocuments(query, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id3);
        }
    }

    // The test verifies correct calcution of system relevance
    void TestCorrectRelevanceOfTheSystem()
    {
        const std::string stop_words = "и в на";
        const size_t doc_count = 3;
        const int doc_id1 = 1, doc_id2 = 2, doc_id3 = 3;
        const std::string content1 = "белый кот и модный ошейник", content2 = "пушистый кот пушистый хвост", content3 = "ухоженный пёс выразительные глаза";
        const std::string query = "пушистый ухоженный кот";

        // Проверка на сортировку по релевантности документов (заранее посчитано)
        {
            SearchServer server("");
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

            ASSERT(fequal(doc0.relevance, 0.650672));
            ASSERT(fequal(doc1.relevance, 0.274653));
            ASSERT(fequal(doc2.relevance, 0.101366));
        }
    }

    // TestSearchServer is entry point for launching tests
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

    // --------- The end of module tests for SearchServer -----------

}
