#include "include/ReadInputFunctions.hpp"

#include <iostream>

std::string ReadLine() 
{
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() 
{
    int result;
    std::cin >> result;     // Input number ...
    ReadLine();             // ... and clearing up the buffer for the next number
    return result;
}
