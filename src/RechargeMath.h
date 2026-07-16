#pragma once

#include <algorithm>
#include <cmath>

namespace O2BoostRecharge::Math
{
    inline constexpr float kEpsilon = 0.0001F;

    [[nodiscard]] inline float AvailableO2(float a_oxygen) noexcept
    {
        return std::isfinite(a_oxygen) ? std::max(0.0F, a_oxygen) : 0.0F;
    }

    [[nodiscard]] inline float SpendableO2(
        float a_oxygen,
        float a_permanentO2,
        float a_reserveFraction) noexcept
    {
        if (!std::isfinite(a_reserveFraction)) {
            return 0.0F;
        }

        // A zero fraction deliberately reproduces the unprotected behavior
        // without requiring a valid capacity.
        if (a_reserveFraction <= 0.0F) {
            return AvailableO2(a_oxygen);
        }

        if (!std::isfinite(a_permanentO2) || a_permanentO2 <= 0.0F) {
            return 0.0F;
        }

        const float reserve =
            a_permanentO2 * std::clamp(a_reserveFraction, 0.0F, 1.0F);
        if (!std::isfinite(reserve)) {
            return 0.0F;
        }

        return std::max(0.0F, AvailableO2(a_oxygen) - reserve);
    }

    [[nodiscard]] inline float FundedFuelLimit(float a_availableO2, float a_o2PerFuel) noexcept
    {
        if (!std::isfinite(a_availableO2) || !std::isfinite(a_o2PerFuel) ||
            a_availableO2 <= 0.0F || a_o2PerFuel <= 0.0F) {
            return 0.0F;
        }

        return a_availableO2 / a_o2PerFuel;
    }

    [[nodiscard]] inline float AllowedRecharge(
        float a_requestedFuel,
        float a_availableO2,
        float a_o2PerFuel) noexcept
    {
        if (!std::isfinite(a_requestedFuel) || a_requestedFuel <= 0.0F) {
            return 0.0F;
        }

        return std::min(a_requestedFuel, FundedFuelLimit(a_availableO2, a_o2PerFuel));
    }

    [[nodiscard]] inline float PositiveIncrease(float a_before, float a_after) noexcept
    {
        if (!std::isfinite(a_before) || !std::isfinite(a_after)) {
            return 0.0F;
        }

        return std::max(0.0F, a_after - a_before);
    }

    [[nodiscard]] inline float O2Cost(float a_fuelGain, float a_o2PerFuel, float a_availableO2) noexcept
    {
        if (!std::isfinite(a_fuelGain) || !std::isfinite(a_o2PerFuel) ||
            a_fuelGain <= 0.0F || a_o2PerFuel <= 0.0F) {
            return 0.0F;
        }

        return std::min(AvailableO2(a_availableO2), a_fuelGain * a_o2PerFuel);
    }

    [[nodiscard]] inline float ActualO2Paid(
        float a_before,
        float a_after,
        float a_requestedCost) noexcept
    {
        if (!std::isfinite(a_before) || !std::isfinite(a_after) ||
            !std::isfinite(a_requestedCost) || a_requestedCost <= 0.0F) {
            return 0.0F;
        }

        return std::clamp(a_before - a_after, 0.0F, a_requestedCost);
    }

    [[nodiscard]] inline float UnfundedFuel(
        float a_actualFuelGain,
        float a_actualO2Paid,
        float a_o2PerFuel) noexcept
    {
        if (!std::isfinite(a_actualFuelGain) || !std::isfinite(a_actualO2Paid) ||
            !std::isfinite(a_o2PerFuel) || a_actualFuelGain <= 0.0F || a_o2PerFuel <= 0.0F) {
            return 0.0F;
        }

        return std::max(0.0F, a_actualFuelGain - (std::max(0.0F, a_actualO2Paid) / a_o2PerFuel));
    }
}
