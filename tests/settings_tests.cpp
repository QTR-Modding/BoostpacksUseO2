#include "Settings.h"

#include <atomic>
#include <barrier>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace
{
    bool Matches(
        const O2BoostRecharge::Settings::Values& a_value,
        const O2BoostRecharge::Settings::Values& a_expected)
    {
        return a_value.o2PerFuel == a_expected.o2PerFuel &&
               a_value.minimumO2ReservePercent == a_expected.minimumO2ReservePercent &&
               a_value.freeInSealedOrBreathable == a_expected.freeInSealedOrBreathable &&
               a_value.disableBoostWhenSuitHidden == a_expected.disableBoostWhenSuitHidden &&
               a_value.debugLogging == a_expected.debugLogging;
    }
}

int main()
{
    using namespace O2BoostRecharge;

    Settings::Values invalid;
    invalid.o2PerFuel = -1.0F;
    invalid.minimumO2ReservePercent = 500.0F;
    Settings::Set(invalid);
    const auto normalized = Settings::Get();
    if (normalized.o2PerFuel != Settings::kDefaultO2PerFuel ||
        normalized.minimumO2ReservePercent != Settings::kMaxMinimumO2ReservePercent) {
        std::cerr << "FAIL: settings normalization\n";
        return EXIT_FAILURE;
    }

    const Settings::Values first{2.0F, 10.0F, true, false, true};
    const Settings::Values second{7.0F, 80.0F, false, true, false};
    Settings::Set(first);

    constexpr std::size_t kIterations = 100000;
    std::barrier startGate(3);
    std::atomic_bool mixed = false;
    std::thread writer([&] {
        startGate.arrive_and_wait();
        for (std::size_t index = 0; index < kIterations; ++index) {
            Settings::Set((index & 1U) == 0 ? second : first);
        }
    });
    std::thread reader([&] {
        startGate.arrive_and_wait();
        for (std::size_t index = 0; index < kIterations; ++index) {
            const auto value = Settings::Get();
            if (!Matches(value, first) && !Matches(value, second)) {
                mixed.store(true, std::memory_order_relaxed);
                return;
            }
        }
    });

    startGate.arrive_and_wait();
    writer.join();
    reader.join();
    if (mixed.load(std::memory_order_relaxed)) {
        std::cerr << "FAIL: mixed settings snapshot\n";
        return EXIT_FAILURE;
    }

    std::cout << "PASS: settings normalization and coherent snapshots\n";
    return EXIT_SUCCESS;
}
