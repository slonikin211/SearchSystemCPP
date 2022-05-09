#pragma once

#include <set>
#include <vector>
#include <string>
#include <string_view>

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings);
std::vector<std::string_view> SplitIntoWords(std::string_view text);
std::vector<std::string_view> SplitIntoWords(const char* text);
std::vector<std::string_view> SplitIntoWords(const std::string& text);