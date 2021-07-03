// Пак Георгий Сергеевич - студент Яндекс Практикума на курсе "Разработчик С++"
// Проект оформлен 03.07.2021
// Создание Фреймворка для тестирования поисковой системы

#include <iostream>

using namespace std;

#include "include/SearchServer.hpp"         // Класс SearchServer из прошлого спринта
#include "include/TestSearchServer.hpp"     // Тестирование SearchServer


int main() {
    Test_SearchServer::TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    return 0;
}
