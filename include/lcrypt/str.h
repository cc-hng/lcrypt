#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <lcrypt/base.h>

namespace lc {

std::vector<std::string_view>  //
str_split(std::string_view str, std::string_view delimiter, bool trim = false);

std::string str_join(const std::vector<std::string_view>& vs, std::string_view delimiter);

std::string str_toupper(std::string_view s);
std::string str_tolower(std::string_view s);

std::string_view str_trim(std::string_view str);

bool str_starts_with(std::string_view s, std::string_view perfix);
bool str_ends_with(std::string_view s, std::string_view perfix);

}  // namespace lc
