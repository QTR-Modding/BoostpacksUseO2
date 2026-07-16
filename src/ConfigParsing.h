#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>

namespace O2BoostRecharge::Config
{
    [[nodiscard]] inline std::string Trim(std::string a_value)
    {
        const auto isSpace = [](unsigned char a_char) {
            return std::isspace(a_char) != 0;
        };
        a_value.erase(
            a_value.begin(),
            std::find_if_not(a_value.begin(), a_value.end(), isSpace));
        a_value.erase(
            std::find_if_not(a_value.rbegin(), a_value.rend(), isSpace).base(),
            a_value.end());
        return a_value;
    }

    [[nodiscard]] inline std::optional<bool> ParseBool(std::string a_value)
    {
        a_value = Trim(a_value);
        std::transform(
            a_value.begin(),
            a_value.end(),
            a_value.begin(),
            [](unsigned char a_char) {
                return static_cast<char>(std::tolower(a_char));
            });

        if (a_value == "1" || a_value == "true" ||
            a_value == "yes" || a_value == "on") {
            return true;
        }
        if (a_value == "0" || a_value == "false" ||
            a_value == "no" || a_value == "off") {
            return false;
        }
        return std::nullopt;
    }

    [[nodiscard]] inline std::optional<float> ParseFiniteFloat(std::string a_value)
    {
        a_value = Trim(a_value);
        if (a_value.empty()) {
            return std::nullopt;
        }

        try {
            std::size_t consumed = 0;
            const float parsed = std::stof(a_value, &consumed);
            if (consumed != a_value.size() || !std::isfinite(parsed)) {
                return std::nullopt;
            }
            return parsed;
        } catch (...) {
            return std::nullopt;
        }
    }
}
