#pragma once
 
// Структура Document - для удобства хранения необходимой информации о документе
struct Document {
    int id;
    double relevance;
    int rating;

    // Конструкторы для инициализации документа
    Document() = default;

    Document(int id, double relevance, int rating);
};

// Перечисляемый класс DocumentStatus для хранения информации о статусе документа
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};