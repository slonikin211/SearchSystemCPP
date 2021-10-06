#include "string_processing.h"

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    for (size_t pos = 0; pos != text.npos; text.remove_prefix(pos + 1)) {
        pos = text.find(' ');
        words.push_back(text.substr(0, pos));
    }
    return words;
}