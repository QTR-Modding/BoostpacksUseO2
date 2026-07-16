#include "ConfigParsing.h"
#include "RechargeMath.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int failures = 0;

    void CheckNear(std::string_view a_name, float a_actual, float a_expected)
    {
        if (std::fabs(a_actual - a_expected) > 0.0001F) {
            std::cerr << "FAIL " << a_name << ": got " << a_actual << ", expected " << a_expected << '\n';
            ++failures;
        }
    }

    void CheckParsedBool(std::string_view a_name, std::string a_value, bool a_expected)
    {
        const auto parsed = O2BoostRecharge::Config::ParseBool(a_value);
        if (!parsed.has_value() || *parsed != a_expected) {
            std::cerr << "FAIL " << a_name << ": boolean was not parsed as expected\n";
            ++failures;
        }
    }

    void CheckInvalidBool(std::string_view a_name, std::string a_value)
    {
        if (O2BoostRecharge::Config::ParseBool(a_value).has_value()) {
            std::cerr << "FAIL " << a_name << ": invalid boolean was accepted\n";
            ++failures;
        }
    }

    void CheckParsedFloat(std::string_view a_name, std::string a_value, float a_expected)
    {
        const auto parsed = O2BoostRecharge::Config::ParseFiniteFloat(a_value);
        if (!parsed.has_value()) {
            std::cerr << "FAIL " << a_name << ": valid float was rejected\n";
            ++failures;
            return;
        }
        CheckNear(a_name, *parsed, a_expected);
    }

    void CheckInvalidFloat(std::string_view a_name, std::string a_value)
    {
        if (O2BoostRecharge::Config::ParseFiniteFloat(a_value).has_value()) {
            std::cerr << "FAIL " << a_name << ": invalid float was accepted\n";
            ++failures;
        }
    }
}

int main()
{
    using namespace O2BoostRecharge::Math;

    CheckParsedBool("true boolean", " true ", true);
    CheckParsedBool("case-insensitive on boolean", "ON", true);
    CheckParsedBool("false boolean", " false ", false);
    CheckParsedBool("numeric false boolean", "0", false);
    CheckInvalidBool("misspelled boolean", "treu");
    CheckInvalidBool("blank boolean", "  ");
    CheckParsedFloat("trimmed reserve percentage", " 20.0 ", 20.0F);
    CheckParsedFloat("negative reserve disables", "-5.0", -5.0F);
    CheckParsedFloat("zero reserve disables", "0", 0.0F);
    CheckInvalidFloat("trailing junk cannot disable reserve", "0oops");
    CheckInvalidFloat("NaN cannot disable reserve", "nan");
    CheckInvalidFloat("blank percentage is invalid", " ");

    CheckNear("positive O2 is available", AvailableO2(100.0F), 100.0F);
    CheckNear("zero O2 is empty", AvailableO2(0.0F), 0.0F);
    CheckNear("negative O2 is empty", AvailableO2(-5.0F), 0.0F);
    CheckNear("NaN O2 is unavailable", AvailableO2(std::numeric_limits<float>::quiet_NaN()), 0.0F);

    CheckNear("20 percent leaves only excess spendable", SpendableO2(100.0F, 100.0F, 0.20F), 80.0F);
    CheckNear("at percentage reserve nothing is spendable", SpendableO2(64.0F, 320.0F, 0.20F), 0.0F);
    CheckNear("below percentage reserve nothing is spendable", SpendableO2(50.0F, 320.0F, 0.20F), 0.0F);
    CheckNear("partial final spend lands at reserve", SpendableO2(64.25F, 320.0F, 0.20F), 0.25F);
    CheckNear("170 capacity reserve", SpendableO2(35.0F, 170.0F, 0.20F), 1.0F);
    CheckNear("220 capacity reserve", SpendableO2(45.0F, 220.0F, 0.20F), 1.0F);
    CheckNear("270 capacity reserve", SpendableO2(55.0F, 270.0F, 0.20F), 1.0F);
    CheckNear("negative fraction is disabled", SpendableO2(30.0F, 320.0F, -0.05F), 30.0F);
    CheckNear(
        "zero fraction does not require capacity",
        SpendableO2(30.0F, std::numeric_limits<float>::quiet_NaN(), 0.0F),
        30.0F);
    CheckNear(
        "invalid capacity fails closed",
        SpendableO2(30.0F, std::numeric_limits<float>::quiet_NaN(), 0.20F),
        0.0F);
    CheckNear(
        "invalid fraction fails closed",
        SpendableO2(30.0F, 100.0F, std::numeric_limits<float>::quiet_NaN()),
        0.0F);
    CheckNear("over-100-percent clamps", SpendableO2(100.0F, 100.0F, 2.0F), 0.0F);
    CheckNear(
        "capacity rise at payment removes spendable O2",
        SpendableO2(65.0F, 400.0F, 0.20F),
        0.0F);

    CheckNear("full funding", AllowedRecharge(0.5F, 100.0F, 1.0F), 0.5F);
    CheckNear("empty suppresses recharge", AllowedRecharge(0.5F, 0.0F, 1.0F), 0.0F);
    CheckNear("partial boundary cap", AllowedRecharge(1.0F, 0.25F, 1.0F), 0.25F);
    CheckNear(
        "percentage boundary honors non-one-to-one ratio",
        AllowedRecharge(1.0F, SpendableO2(64.5F, 320.0F, 0.20F), 2.0F),
        0.25F);
    CheckNear("ratio cap", AllowedRecharge(10.0F, 6.0F, 2.0F), 3.0F);
    CheckNear("negative is not recharge", AllowedRecharge(-4.0F, 100.0F, 1.0F), 0.0F);
    CheckNear("invalid ratio funds nothing", AllowedRecharge(4.0F, 100.0F, 0.0F), 0.0F);

    CheckNear("actual gain", PositiveIncrease(40.0F, 43.5F), 3.5F);
    CheckNear("consumption is not gain", PositiveIncrease(43.5F, 40.0F), 0.0F);
    CheckNear("near-full cost uses actual gain", O2Cost(0.4F, 1.0F, 100.0F), 0.4F);
    CheckNear("cost cannot exceed O2", O2Cost(4.0F, 2.0F, 3.0F), 3.0F);

    CheckNear("normal debit paid", ActualO2Paid(10.0F, 7.0F, 3.0F), 3.0F);
    CheckNear("clamped debit paid", ActualO2Paid(1.0F, 0.0F, 2.0F), 1.0F);
    CheckNear("O2 increase is not payment", ActualO2Paid(5.0F, 7.0F, 2.0F), 0.0F);
    CheckNear("overdraw cannot overpay", ActualO2Paid(1.0F, -5.0F, 1.0F), 1.0F);

    CheckNear("fully paid has no rollback", UnfundedFuel(4.0F, 4.0F, 1.0F), 0.0F);
    CheckNear("partial payment rollback", UnfundedFuel(4.0F, 2.0F, 1.0F), 2.0F);
    CheckNear("ratio-aware rollback", UnfundedFuel(4.0F, 2.0F, 2.0F), 3.0F);
    CheckNear("zero payment rolls back all", UnfundedFuel(4.0F, 0.0F, 1.0F), 4.0F);

    const float initiallyAllowed = AllowedRecharge(
        10.0F,
        SpendableO2(80.0F, 320.0F, 0.20F),
        1.0F);
    const float costAfterCapacityRise = O2Cost(
        initiallyAllowed,
        1.0F,
        SpendableO2(65.0F, 400.0F, 0.20F));
    CheckNear("capacity rise prevents payment", costAfterCapacityRise, 0.0F);
    CheckNear(
        "capacity rise rolls back all newly unfunded fuel",
        UnfundedFuel(initiallyAllowed, costAfterCapacityRise, 1.0F),
        initiallyAllowed);

    const float allowedBeforeExternalDrain = AllowedRecharge(
        10.0F,
        SpendableO2(70.0F, 320.0F, 0.20F),
        1.0F);
    const float costAfterExternalDrain = O2Cost(
        allowedBeforeExternalDrain,
        1.0F,
        SpendableO2(64.5F, 320.0F, 0.20F));
    CheckNear("payment re-cap after external drain", costAfterExternalDrain, 0.5F);
    CheckNear(
        "external drain rolls back the unpaid remainder",
        UnfundedFuel(allowedBeforeExternalDrain, costAfterExternalDrain, 1.0F),
        5.5F);

    if (failures != 0) {
        std::cerr << failures << " recharge math test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: all O2-funded recharge boundary and config tests\n";
    return 0;
}
