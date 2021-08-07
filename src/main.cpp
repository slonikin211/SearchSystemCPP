// Пак Георгий Сергеевич - студент Яндекс Практикума на курсе "Разработчик С++"
// Проект оформлен 03.07.2021
// Создание поисковой системы с использованием TF-IDF для ранжирования документов
// Создание Фреймворка для тестирования поисковой системы
// Обработка исключений
// Использование пагинатора для "страниц" по результатам поиска
// Распределение кода по файлам


/*
    Примечание:
    Поскольку в проекте использовались шаблоны, файлы .hpp содержат также реализацию
    шаблонных классов => .cpp файлы с реализацией для них не нужны

    В файле SearchServer в примечении описана проблема связанная с линковкой. Ее не смог решить
    самостоятельно. Проверьте пожалуйста!
*/

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
