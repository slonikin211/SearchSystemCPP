#include "ReadInputFunctions.hpp"

#include <string>
#include <iostream>

// Считывает строку
std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// Считывает строку с числом
int ReadLineWithNumber() {
    int result;
    std::cin >> result;  // Вводим число
    ReadLine();     // ...и очищаем буфер для последующего ввода
    return result;
}
