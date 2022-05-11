#pragma once

#include <set>
#include <vector>
#include <string>
#include <string_view>

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings);
std::vector<std::string_view> SplitIntoWords(std::string_view text);
