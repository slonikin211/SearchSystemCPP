#pragma once
 
// Document - structure for storaging data of documents
struct Document 
{
    int id;
    double relevance;
    int rating;

    // Constructor for document initialization
    Document() = default;

    Document(int id, double relevance, int rating);
};

// Enum Class DocumentStatus with document states
enum class DocumentStatus 
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};