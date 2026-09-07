#include "Settings.h"

#include "ConfigParsing.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace O2BoostRecharge::Settings
{
    namespace
    {
        constexpr auto kRelativePath = "Data/SFSE/Plugins/O2BoostRecharge.ini";

        std::atomic_uint64_t revision = 0;
        std::atomic<float> o2PerFuel = kDefaultO2PerFuel;
        std::atomic<float> minimumO2ReservePercent = kDefaultMinimumO2ReservePercent;
        std::atomic_bool freeInSealedOrBreathable = true;
        std::atomic_bool disableBoostWhenSuitHidden = true;
        std::atomic_bool debugLogging = false;

        [[nodiscard]] std::filesystem::path GetPath()
        {
            std::array<wchar_t, 32768> executable{};
            const auto length = ::GetModuleFileNameW(
                nullptr,
                executable.data(),
                static_cast<DWORD>(executable.size()));
            if (length == 0 || length >= executable.size()) {
                return kRelativePath;
            }

            return std::filesystem::path(executable.data(), executable.data() + length)
                       .parent_path() /
                   kRelativePath;
        }

        [[nodiscard]] Values Normalize(const Values& a_values) noexcept
        {
            Values result = a_values;
            result.o2PerFuel = std::isfinite(result.o2PerFuel) && result.o2PerFuel > 0.0F ?
                                   std::clamp(result.o2PerFuel, kMinO2PerFuel, kMaxO2PerFuel) :
                                   kDefaultO2PerFuel;
            result.minimumO2ReservePercent = std::isfinite(result.minimumO2ReservePercent) ?
                                                 std::clamp(
                                                     result.minimumO2ReservePercent,
                                                     0.0F,
                                                     kMaxMinimumO2ReservePercent) :
                                                 kDefaultMinimumO2ReservePercent;
            return result;
        }

        [[nodiscard]] std::string Lower(std::string a_value)
        {
            std::transform(
                a_value.begin(),
                a_value.end(),
                a_value.begin(),
                [](unsigned char a_char) { return static_cast<char>(std::tolower(a_char)); });
            return a_value;
        }

        void LoadBool(const char* a_key, const std::string& a_value, bool& a_setting)
        {
            const auto parsed = Config::ParseBool(a_value);
            if (parsed.has_value()) {
                a_setting = *parsed;
            } else {
                logger::warn("Ignoring invalid boolean {}={}", a_key, a_value);
            }
        }
    }

    Values Get() noexcept
    {
        Values values;
        for (;;) {
            const auto before = revision.load(std::memory_order_acquire);
            if ((before & 1U) != 0) {
                continue;
            }

            values.o2PerFuel = o2PerFuel.load(std::memory_order_relaxed);
            values.minimumO2ReservePercent = minimumO2ReservePercent.load(std::memory_order_relaxed);
            values.freeInSealedOrBreathable = freeInSealedOrBreathable.load(std::memory_order_relaxed);
            values.disableBoostWhenSuitHidden = disableBoostWhenSuitHidden.load(std::memory_order_relaxed);
            values.debugLogging = debugLogging.load(std::memory_order_relaxed);

            if (revision.load(std::memory_order_acquire) == before) {
                return values;
            }
        }
    }

    void Set(const Values& a_values) noexcept
    {
        const auto values = Normalize(a_values);
        revision.fetch_add(1, std::memory_order_acq_rel);
        o2PerFuel.store(values.o2PerFuel, std::memory_order_relaxed);
        minimumO2ReservePercent.store(values.minimumO2ReservePercent, std::memory_order_relaxed);
        freeInSealedOrBreathable.store(values.freeInSealedOrBreathable, std::memory_order_relaxed);
        disableBoostWhenSuitHidden.store(values.disableBoostWhenSuitHidden, std::memory_order_relaxed);
        debugLogging.store(values.debugLogging, std::memory_order_relaxed);
        revision.fetch_add(1, std::memory_order_release);
    }

    void Reset() noexcept
    {
        Set(Values{});
    }

    bool Load()
    {
        const auto path = GetPath();
        std::ifstream input(path);
        if (!input) {
            logger::warn("Config not found at {}; using current values", path.string());
            return false;
        }

        Values values;
        std::string section;
        std::string line;
        while (std::getline(input, line)) {
            line = Config::Trim(line);
            if (line.empty() || line.starts_with(';') || line.starts_with('#')) {
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                section = Lower(Config::Trim(line.substr(1, line.size() - 2)));
                continue;
            }

            const auto separator = line.find('=');
            if (separator == std::string::npos || section != "general") {
                continue;
            }

            const auto key = Lower(Config::Trim(line.substr(0, separator)));
            auto value = Config::Trim(line.substr(separator + 1));
            if (const auto comment = value.find_first_of(";#"); comment != std::string::npos) {
                value = Config::Trim(value.substr(0, comment));
            }

            if (key == "fo2perfuel") {
                const auto parsed = Config::ParseFiniteFloat(value);
                if (parsed.has_value() && *parsed > 0.0F) {
                    values.o2PerFuel = *parsed;
                } else {
                    logger::warn("Ignoring non-positive fO2PerFuel={}", value);
                }
            } else if (key == "fminimumo2reservepercent") {
                const auto parsed = Config::ParseFiniteFloat(value);
                if (parsed.has_value()) {
                    values.minimumO2ReservePercent = *parsed;
                } else {
                    logger::warn("Ignoring invalid fMinimumO2ReservePercent={}", value);
                }
            } else if (key == "bfreeinsealedorbreathable") {
                LoadBool("bFreeInSealedOrBreathable", value, values.freeInSealedOrBreathable);
            } else if (key == "bdisableboostwhensuithidden") {
                LoadBool("bDisableBoostWhenSuitHidden", value, values.disableBoostWhenSuitHidden);
            } else if (key == "bdebuglogging") {
                LoadBool("bDebugLogging", value, values.debugLogging);
            }
        }

        Set(values);
        const auto loaded = Get();
        logger::info(
            "Config: fO2PerFuel={}, fMinimumO2ReservePercent={}, "
            "bFreeInSealedOrBreathable={}, bDisableBoostWhenSuitHidden={}, bDebugLogging={}",
            loaded.o2PerFuel,
            loaded.minimumO2ReservePercent,
            loaded.freeInSealedOrBreathable,
            loaded.disableBoostWhenSuitHidden,
            loaded.debugLogging);
        return true;
    }

    bool Save()
    {
        const auto values = Get();
        const auto path = GetPath();
        auto temporary = path;
        temporary += ".tmp";

        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            logger::error("Could not open {} for writing", temporary.string());
            return false;
        }

        output << "[General]\n"
               << "fO2PerFuel=" << values.o2PerFuel << '\n'
               << "fMinimumO2ReservePercent=" << values.minimumO2ReservePercent << '\n'
               << "bFreeInSealedOrBreathable=" << (values.freeInSealedOrBreathable ? 1 : 0) << '\n'
               << "bDisableBoostWhenSuitHidden=" << (values.disableBoostWhenSuitHidden ? 1 : 0) << '\n'
               << "bDebugLogging=" << (values.debugLogging ? 1 : 0) << '\n';
        output.close();

        if (!output) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            logger::error("Could not write {}", temporary.string());
            return false;
        }

        if (!::MoveFileExW(
                temporary.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            const auto error = ::GetLastError();
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            logger::error("Could not replace {} (Win32 error {})", path.string(), error);
            return false;
        }

        return true;
    }
}
