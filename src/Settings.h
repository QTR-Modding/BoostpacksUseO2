#pragma once

namespace O2BoostRecharge::Settings
{
    inline constexpr float kDefaultO2PerFuel = 1.0F;
    inline constexpr float kMinO2PerFuel = 0.001F;
    inline constexpr float kMaxO2PerFuel = 1000.0F;
    inline constexpr float kDefaultMinimumO2ReservePercent = 20.0F;
    inline constexpr float kMaxMinimumO2ReservePercent = 100.0F;

    struct Values
    {
        float o2PerFuel = kDefaultO2PerFuel;
        float minimumO2ReservePercent = kDefaultMinimumO2ReservePercent;
        bool freeInSealedOrBreathable = true;
        bool disableBoostWhenSuitHidden = true;
        bool debugLogging = false;
    };

    [[nodiscard]] Values Get() noexcept;
    void Set(const Values& a_values) noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool Load();
    [[nodiscard]] bool Save();
}
