#include "Menu.h"

#include "Settings.h"

#include <SFSEMCP/SFSEMenuFramework.hpp>

#include <string>

namespace O2BoostRecharge::Menu
{
    namespace
    {
        bool dirty = false;
        std::string status;

        void ApplyIfChanged(Settings::Values& a_values, bool a_changed)
        {
            if (a_changed) {
                Settings::Set(a_values);
                dirty = true;
                status.clear();
            }
        }

        void __stdcall DrawSettings()
        {
            auto values = Settings::Get();

            bool changed = ImGuiMCP::SliderFloat(
                "O2 spent per fuel point",
                &values.o2PerFuel,
                Settings::kMinO2PerFuel,
                Settings::kMaxO2PerFuel,
                "%.3f",
                ImGuiMCP::ImGuiSliderFlags_Logarithmic | ImGuiMCP::ImGuiSliderFlags_AlwaysClamp);
            changed |= ImGuiMCP::SliderFloat(
                "Minimum O2 reserve",
                &values.minimumO2ReservePercent,
                0.0F,
                Settings::kMaxMinimumO2ReservePercent,
                "%.1f%%",
                ImGuiMCP::ImGuiSliderFlags_AlwaysClamp);
            changed |= ImGuiMCP::Checkbox(
                "Free recharge in sealed or breathable areas",
                &values.freeInSealedOrBreathable);
            changed |= ImGuiMCP::Checkbox(
                "Disable boosting while the spacesuit is hidden",
                &values.disableBoostWhenSuitHidden);
            changed |= ImGuiMCP::Checkbox("Debug logging", &values.debugLogging);
            ApplyIfChanged(values, changed);

            ImGuiMCP::Separator();
            if (ImGuiMCP::Button("Save")) {
                if (Settings::Save()) {
                    dirty = false;
                    status = "Settings saved.";
                } else {
                    status = "Could not save O2BoostRecharge.ini.";
                }
            }

            ImGuiMCP::SameLine();
            if (ImGuiMCP::Button("Reset to defaults")) {
                Settings::Reset();
                dirty = true;
                status.clear();
            }

            if (!status.empty()) {
                ImGuiMCP::TextUnformatted(status.c_str());
            }
            if (dirty) {
                ImGuiMCP::TextDisabled("Changes are live but not saved.");
            }
        }
    }

    void Register()
    {
        SFSEMenuFramework::SetSection("Boostpacks Use O2");
        SFSEMenuFramework::AddSectionItem("Settings", DrawSettings);
    }
}
