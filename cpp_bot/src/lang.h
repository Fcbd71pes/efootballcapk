// lang.h — Bilingual string lookup
#pragma once
#include <string>
#include <map>
#include <unordered_map>

namespace lang {
    struct LangString { std::string bn, en; };
    extern std::unordered_map<std::string, LangString> STRINGS;
    std::string t(const std::string& key,
                  const std::string& language = "en",
                  const std::map<std::string, std::string>& kwargs = {});
    std::string fmt(double val, int decimals = 2);
}
