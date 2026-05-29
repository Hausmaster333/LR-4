#ifndef STRING_OPERATIONS_H
#define STRING_OPERATIONS_H

#include "core/sequence.h"
#include <string>
#include <functional>
#include <cctype>

namespace str_ops {

    inline std::function<std::string(const std::string&)> to_upper() { // Привести к заглавным
        return [](const std::string& s) {
            std::string result = s;

            for (size_t i = 0; i < result.size(); i++) {
                result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
            }

            return result;
        };
    }

    inline std::function<std::string(const std::string&)> to_lower() { // Привести к строчным
        return [](const std::string& s) {
            std::string result = s;

            for (size_t i = 0; i < result.size(); i++) {
                result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
            }

            return result;
        };
    }

    inline std::function<std::string(const std::string&)> trim() {
        return [](const std::string& s) {
            size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
                start++;
            }

            size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
                end--;
            }

            return s.substr(start, end - start);
        };
    }

    inline std::function<std::string(const std::string&)> substr(int start, int count = -1) {
        return [start, count](const std::string& s) {
            if (start >= static_cast<int>(s.size())) return std::string();
            if (count < 0) return s.substr(static_cast<size_t>(start));

            return s.substr(static_cast<size_t>(start), static_cast<size_t>(count));
        };
    }

    inline std::function<std::string(const std::string&)> replace_all(const std::string& from, const std::string& to) {
        return [from, to](const std::string& s) {
            if (from.empty()) return s;

            std::string result = s;
            size_t pos = 0;

            while ((pos = result.find(from, pos)) != std::string::npos) {
                result.replace(pos, from.size(), to);
                pos += to.size();
            }

            return result;
        };
    }

    inline std::function<std::string(const std::string&)> prepend(const std::string& prefix) {
        return [prefix](const std::string& s) { return prefix + s; };
    }

    inline std::function<std::string(const std::string&)> append(const std::string& suffix) {
        return [suffix](const std::string& s) { return s + suffix; };
    }

    inline std::function<bool(const std::string&)> starts_with(const std::string& prefix) {
        return [prefix](const std::string& s) {
            return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
        };
    }

    inline std::function<bool(const std::string&)> ends_with(const std::string& suffix) {
        return [suffix](const std::string& s) {
            return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        };
    }

    inline std::function<bool(const std::string&)> contains(const std::string& substring) {
        return [substring](const std::string& s) {
            return s.find(substring) != std::string::npos;
        };
    }

    inline std::function<bool(const std::string&)> min_length(int n) {
        return [n](const std::string& s) {
            return static_cast<int>(s.size()) >= n;
        };
    }

    inline std::function<bool(const std::string&)> is_not_empty() {
        return [](const std::string& s) { return !s.empty(); };
    }

    inline std::string join(const MutableArraySequence<std::string>* arr, const std::string& delimiter) {
        if (arr == nullptr || arr->get_count() == 0) return "";

        std::string result = arr->get(0);

        for (int i = 1; i < arr->get_count(); i++) {
            result += delimiter;
            result += arr->get(i);
        }

        return result;
    }

}

#endif
