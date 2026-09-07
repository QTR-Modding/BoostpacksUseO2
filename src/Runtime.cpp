#include "RechargeMath.h"
#include "Runtime.h"
#include "Settings.h"

#include <atomic>
#include <cmath>
#include <cstdint>

namespace GameConditions
{
    // Starfield 1.16.244 native ConditionForm.IsTrue implementation. It builds
    // the runtime's 0x70-byte parameter block and evaluates the complete list,
    // avoiding stale CommonLibSF layouts for both structures.
    using is_true_t = bool (*)(
        void*,
        std::uint32_t,
        RE::BGSConditionForm*,
        RE::TESObjectREFR*,
        RE::TESObjectREFR*);

    std::atomic_bool sealedOrBreathable = false;
    RE::BGSConditionForm* environmentCondition = nullptr;
    RE::PlayerCharacter* player = nullptr;
    bool environmentTaskInstalled = false;

    bool Evaluate(RE::BGSConditionForm* a_condition, RE::TESObjectREFR* a_subject)
    {
        if (!a_condition || !a_subject) {
            return false;
        }

        static REL::Relocation<is_true_t> isTrue{ REL::ID(114781) };
        return isTrue(nullptr, 0, a_condition, a_subject, nullptr);
    }

    bool StartEnvironmentCache(
        RE::BGSConditionForm* a_condition,
        RE::PlayerCharacter* a_player)
    {
        if (environmentTaskInstalled) {
            return true;
        }

        const auto* tasks = SFSE::GetTaskInterface();
        if (!a_condition || !a_player || !tasks) {
            logger::error(
                "Atmosphere cache not started: Condition={}, Player={}, Tasks={}",
                static_cast<const void*>(a_condition),
                static_cast<const void*>(a_player),
                static_cast<const void*>(tasks));
            return false;
        }

        environmentCondition = a_condition;
        player = a_player;
        sealedOrBreathable.store(false, std::memory_order_release);
        tasks->AddPermanentTask([] {
            const bool result = player && player->parentCell &&
                                Evaluate(environmentCondition, player);
            sealedOrBreathable.store(result, std::memory_order_release);
        });
        environmentTaskInstalled = true;
        logger::info("Started main-thread sealed/breathable atmosphere cache");
        return true;
    }

    bool IsSealedOrBreathable()
    {
        return sealedOrBreathable.load(std::memory_order_acquire);
    }
}

namespace RechargeHook
{
    // Starfield 1.16.244 ValueModifierEffect vtable slot 0x1E. The fourth
    // argument can override the effect's ActorValueInfo stored at +0x98.
    using function_t = void (*)(void*, RE::Actor*, float, RE::ActorValueInfo*);
    using mod_actor_value_no_source_t = void (*)(
        RE::ActorValueOwner*,
        RE::ACTOR_VALUE_MODIFIER,
        const RE::ActorValueInfo&,
        float);

    constexpr std::size_t kApplyActorValueSlot = 0x1E;
    constexpr std::size_t kModActorValueNoSourceSlot = 0x07;
    constexpr std::ptrdiff_t kEffectActorValueOffset = 0x98;
    constexpr std::uint32_t kMaxDebugSamples = 120;

    REL::Relocation<function_t> original;
    RE::ActorValueInfo* boostFuel = nullptr;
    RE::ActorValueInfo* oxygen = nullptr;
    RE::BGSConditionForm* sealedOrBreathableCondition = nullptr;
    RE::PlayerCharacter* player = nullptr;
    std::atomic_uint32_t debugSamples = 0;
    bool installed = false;

    void ModActorValueNoSource(
        RE::Actor* a_target,
        RE::ACTOR_VALUE_MODIFIER a_modifier,
        const RE::ActorValueInfo& a_info,
        float a_delta)
    {
        // The CommonLibSF overloads compile to the opposite vtable entries on
        // this revision. Call the runtime-verified no-source slot directly.
        // Starfield's native queued O2-cost path uses this same slot (0x07).
        auto* owner = static_cast<RE::ActorValueOwner*>(a_target);
        auto* vtable = *reinterpret_cast<std::uintptr_t**>(owner);
        auto function = reinterpret_cast<mod_actor_value_no_source_t>(
            vtable[kModActorValueNoSourceSlot]);
        function(owner, a_modifier, a_info, a_delta);
    }

    RE::ActorValueInfo* ResolveActorValue(void* a_effect, RE::ActorValueInfo* a_override)
    {
        if (a_override) {
            return a_override;
        }

        const auto address = reinterpret_cast<std::uintptr_t>(a_effect) + kEffectActorValueOffset;
        return *reinterpret_cast<RE::ActorValueInfo**>(address);
    }

    void LogSample(
        bool a_enabled,
        float a_requested,
        float a_allowed,
        float a_reserveFraction,
        float a_oxygenCapacity,
        float a_fuelBefore,
        float a_fuelAfter,
        float a_oxygenBefore,
        float a_oxygenAfter,
        float a_paid,
        float a_rollback)
    {
        if (a_enabled &&
            debugSamples.fetch_add(1, std::memory_order_relaxed) < kMaxDebugSamples) {
            logger::info(
                "Recharge sample: requested={}, allowed={}, reserve={}%, capacity={}, "
                "fuel {} -> {}, O2 {} -> {}, paid={}, rollback={}",
                a_requested,
                a_allowed,
                a_reserveFraction * 100.0F,
                a_oxygenCapacity,
                a_fuelBefore,
                a_fuelAfter,
                a_oxygenBefore,
                a_oxygenAfter,
                a_paid,
                a_rollback);
        }
    }

    void Apply(void* a_effect, RE::Actor* a_target, float a_delta, RE::ActorValueInfo* a_overrideActorValue)
    {
        if (!a_effect || !a_target) {
            return;
        }

        if (a_target != player ||
            ResolveActorValue(a_effect, a_overrideActorValue) != boostFuel ||
            !std::isfinite(a_delta) || a_delta <= 0.0F) {
            original(a_effect, a_target, a_delta, a_overrideActorValue);
            return;
        }

        const auto settings = O2BoostRecharge::Settings::Get();
        if (settings.freeInSealedOrBreathable &&
            GameConditions::IsSealedOrBreathable()) {
            original(a_effect, a_target, a_delta, a_overrideActorValue);
            return;
        }

        const float fuelBefore = a_target->GetActorValue(*boostFuel);
        const float oxygenBefore = a_target->GetActorValue(*oxygen);
        // Starfield's GetValuePercent and ObjectReference.GetValuePercentage
        // both use GetPermanentActorValue as the denominator.
        const float oxygenCapacityBefore =
            a_target->GetPermanentActorValue(*oxygen);
        const float reserveFraction = settings.minimumO2ReservePercent * 0.01F;
        const float spendableO2 =
            O2BoostRecharge::Math::SpendableO2(
                oxygenBefore,
                oxygenCapacityBefore,
                reserveFraction);

        // If either actor value cannot be accounted, suppress this one positive
        // recharge update instead of permitting an unpriced restoration.
        const float allowed = std::isfinite(fuelBefore) && std::isfinite(oxygenBefore) ?
                                  O2BoostRecharge::Math::AllowedRecharge(
                                      a_delta,
                                      spendableO2,
                                      settings.o2PerFuel) :
                                  0.0F;

        original(a_effect, a_target, allowed, a_overrideActorValue);

        float fuelAfter = a_target->GetActorValue(*boostFuel);
        float actualGain = O2BoostRecharge::Math::PositiveIncrease(fuelBefore, fuelAfter);
        const float fundedLimit = O2BoostRecharge::Math::FundedFuelLimit(
            spendableO2,
            settings.o2PerFuel);
        float rollback = 0.0F;

        // Defensive postcondition for another hook changing the requested
        // delta: the gain cannot exceed what pre-update O2 could fund.
        if (actualGain > fundedLimit + O2BoostRecharge::Math::kEpsilon) {
            const float excess = actualGain - fundedLimit;
            ModActorValueNoSource(
                a_target,
                RE::ACTOR_VALUE_MODIFIER::kDamage,
                *boostFuel,
                -excess);
            rollback += excess;
            fuelAfter = a_target->GetActorValue(*boostFuel);
            actualGain = O2BoostRecharge::Math::PositiveIncrease(fuelBefore, fuelAfter);
        }

        float oxygenAfter = oxygenBefore;
        float actualPaid = 0.0F;
        if (actualGain > 0.0F) {
            // Re-read just before payment in case a synchronous handler changed
            // O2 or its permanent capacity while fuel was being applied.
            const float oxygenAtPayment = a_target->GetActorValue(*oxygen);
            const float oxygenCapacityAtPayment =
                a_target->GetPermanentActorValue(*oxygen);
            const float spendableAtPayment =
                O2BoostRecharge::Math::SpendableO2(
                    oxygenAtPayment,
                    oxygenCapacityAtPayment,
                    reserveFraction);
            const float requestedCost = O2BoostRecharge::Math::O2Cost(
                actualGain,
                settings.o2PerFuel,
                spendableAtPayment);

            if (requestedCost > 0.0F) {
                // Negative kDamage is Starfield's resource-debit direction;
                // use the runtime-validated no-source helper above.
                ModActorValueNoSource(
                    a_target,
                    RE::ACTOR_VALUE_MODIFIER::kDamage,
                    *oxygen,
                    -requestedCost);
            }

            oxygenAfter = a_target->GetActorValue(*oxygen);
            actualPaid = O2BoostRecharge::Math::ActualO2Paid(
                oxygenAtPayment,
                oxygenAfter,
                requestedCost);

            // Strict accounting: if a clamp, god mode, or another hook prevents
            // part of the O2 debit, remove the corresponding unpaid fuel.
            const float unpaidFuel = O2BoostRecharge::Math::UnfundedFuel(
                actualGain,
                actualPaid,
                settings.o2PerFuel);
            if (unpaidFuel > 0.0F) {
                ModActorValueNoSource(
                    a_target,
                    RE::ACTOR_VALUE_MODIFIER::kDamage,
                    *boostFuel,
                    -unpaidFuel);
                rollback += unpaidFuel;
                fuelAfter = a_target->GetActorValue(*boostFuel);
            }
        }

        LogSample(
            settings.debugLogging,
            a_delta,
            allowed,
            reserveFraction,
            oxygenCapacityBefore,
            fuelBefore,
            fuelAfter,
            oxygenBefore,
            oxygenAfter,
            actualPaid,
            rollback);
    }

    bool Install()
    {
        if (installed) {
            logger::warn("Recharge hook was already installed");
            return true;
        }

        boostFuel = RE::TESForm::LookupByID<RE::ActorValueInfo>(0x00024021);
        oxygen = RE::TESForm::LookupByID<RE::ActorValueInfo>(0x000002D5);
        sealedOrBreathableCondition =
            RE::TESForm::LookupByID<RE::BGSConditionForm>(0x002FAA06);
        player = RE::PlayerCharacter::GetSingleton();

        const bool atmosphereReady =
            GameConditions::StartEnvironmentCache(sealedOrBreathableCondition, player);

        if (!boostFuel || !oxygen || !player || !atmosphereReady) {
            logger::error(
                "Recharge hook not installed: BoostpackFuel={}, Oxygen={}, "
                "SealedOrBreathable={}, Player={}, AtmosphereReady={}",
                static_cast<const void*>(boostFuel),
                static_cast<const void*>(oxygen),
                static_cast<const void*>(sealedOrBreathableCondition),
                static_cast<const void*>(player),
                atmosphereReady);
            return false;
        }

        REL::Relocation vtable{ RE::VTABLE::ValueModifierEffect[0] };
        original = vtable.write_vfunc(kApplyActorValueSlot, Apply);
        installed = true;
        logger::info(
            "Installed player BoostpackFuel recharge hook at ValueModifierEffect slot 0x{:X}",
            kApplyActorValueSlot);
        return true;
    }
}

namespace NoMagicBoost
{
    using jump_press_event_t = RE::PlayerControls::PlayerJumpPressEvent;
    using jump_press_source_t = RE::BSTEventSource<jump_press_event_t>;
    using on_jump_press_t = RE::BSEventNotifyControl (*)(
        void*,
        const jump_press_event_t&,
        jump_press_source_t*);
    using on_button_event_t = void (*)(void*, const RE::ButtonEvent*);

    enum class SequenceState : std::uint8_t
    {
        kIdle,
        kSuppressedUntilRelease
    };

    constexpr std::size_t kProcessJumpPressSlot = 0x01;
    constexpr std::size_t kOnButtonEventSlot = 0x08;

    REL::Relocation<on_jump_press_t> originalOnJumpPress;
    REL::Relocation<on_button_event_t> originalOnButtonEvent;
    RE::ActorValueInfo* boostpackActive = nullptr;
    RE::BGSConditionForm* shouldShowSpacesuit = nullptr;
    RE::PlayerCharacter* player = nullptr;
    std::atomic<SequenceState> sequence = SequenceState::kIdle;
    bool installed = false;

    void Reset()
    {
        sequence.store(SequenceState::kIdle, std::memory_order_release);
    }

    RE::BSEventNotifyControl OnJumpPress(
        void* a_sink,
        const jump_press_event_t& a_event,
        jump_press_source_t* a_source)
    {
        if (!a_sink) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (!O2BoostRecharge::Settings::Get().disableBoostWhenSuitHidden) {
            return originalOnJumpPress(a_sink, a_event, a_source);
        }

        if (player && shouldShowSpacesuit &&
            !GameConditions::Evaluate(shouldShowSpacesuit, player)) {
            // Veto only JetpackEffect's sink. The event dispatcher continues
            // to the ordinary ground-jump listeners.
            return RE::BSEventNotifyControl::kContinue;
        }

        return originalOnJumpPress(a_sink, a_event, a_source);
    }

    void MarkContinue(const RE::ButtonEvent* a_event)
    {
        const_cast<RE::ButtonEvent*>(a_event)->status =
            RE::InputEvent::Status::kContinue;
    }

    void SendSyntheticRelease(void* a_handler, const RE::ButtonEvent& a_event)
    {
        RE::ButtonEvent release{ a_event };
        release.next = nullptr;
        release.status = RE::InputEvent::Status::kUnhandled;
        release.value = 0.0F;
        release.heldDownSecs = 0.0F;
        originalOnButtonEvent(a_handler, &release);
    }

    void OnButtonEvent(void* a_handler, const RE::ButtonEvent* a_event)
    {
        if (!a_handler || !a_event) {
            Reset();
            return;
        }

        if (!player || !shouldShowSpacesuit || !boostpackActive) {
            Reset();
            originalOnButtonEvent(a_handler, a_event);
            return;
        }

        if (!std::isfinite(a_event->value) ||
            !std::isfinite(a_event->heldDownSecs) ||
            a_event->value < 0.0F) {
            originalOnButtonEvent(a_handler, a_event);
            return;
        }

        const bool released = a_event->value == 0.0F;
        const bool initialDown = a_event->value > 0.0F &&
                                 a_event->heldDownSecs == 0.0F;
        auto current = sequence.load(std::memory_order_acquire);

        // A fresh press repairs stale state left by lost focus or a missing
        // device-release event.
        if (initialDown) {
            sequence.store(SequenceState::kIdle, std::memory_order_release);
            originalOnButtonEvent(a_handler, a_event);
            return;
        }

        // Finish consuming a sequence that this hook already terminated before
        // allowing a newly disabled live setting to return control to vanilla.
        if (current == SequenceState::kSuppressedUntilRelease) {
            MarkContinue(a_event);
            if (released) {
                sequence.store(SequenceState::kIdle, std::memory_order_release);
            }
            return;
        }

        if (!O2BoostRecharge::Settings::Get().disableBoostWhenSuitHidden) {
            originalOnButtonEvent(a_handler, a_event);
            return;
        }

        if (released) {
            originalOnButtonEvent(a_handler, a_event);
            sequence.store(SequenceState::kIdle, std::memory_order_release);
            return;
        }

        const bool hidden = !GameConditions::Evaluate(shouldShowSpacesuit, player);
        const float activeValue = player->GetActorValue(*boostpackActive);
        const bool active = std::isfinite(activeValue) && activeValue > 0.5F;

        // A visible boost can cross into a suit-hidden location. Terminate
        // only actual thrust through vanilla's release branch; hidden ground
        // jumps retain their normal hold behavior.
        if (hidden && active) {
            SendSyntheticRelease(a_handler, *a_event);
            MarkContinue(a_event);
            sequence.store(
                SequenceState::kSuppressedUntilRelease,
                std::memory_order_release);
            return;
        }

        originalOnButtonEvent(a_handler, a_event);
    }

    bool Install()
    {
        if (installed) {
            logger::warn("Suit-hidden boost hooks were already installed");
            return true;
        }

        boostpackActive =
            RE::TESForm::LookupByID<RE::ActorValueInfo>(0x0002466E);
        shouldShowSpacesuit =
            RE::TESForm::LookupByID<RE::BGSConditionForm>(0x00194ABF);
        player = RE::PlayerCharacter::GetSingleton();

        if (!boostpackActive || !shouldShowSpacesuit || !player) {
            logger::error(
                "Suit-hidden boost hooks not installed: BoostpackActive={}, "
                "ShouldShowSuit={}, Player={}",
                static_cast<const void*>(boostpackActive),
                static_cast<const void*>(shouldShowSpacesuit),
                static_cast<const void*>(player));
            return false;
        }

        REL::Relocation jetpackPressSinkVtable{ REL::ID(448594) };
        originalOnJumpPress =
            jetpackPressSinkVtable.write_vfunc(
                kProcessJumpPressSlot,
                OnJumpPress);

        REL::Relocation jumpHandlerVtable{ REL::ID(433587) };
        originalOnButtonEvent =
            jumpHandlerVtable.write_vfunc(kOnButtonEventSlot, OnButtonEvent);

        Reset();
        installed = true;
        logger::info(
            "Installed player suit-hidden boost hooks at JetpackEffect press "
            "sink slot 0x{:X} and JumpHandler slot 0x{:X}",
            kProcessJumpPressSlot,
            kOnButtonEventSlot);
        return true;
    }
}

namespace O2BoostRecharge::Runtime
{
    bool Install()
    {
        const bool rechargeInstalled = RechargeHook::Install();
        const bool noMagicInstalled = NoMagicBoost::Install();
        if (!rechargeInstalled || !noMagicInstalled) {
            logger::error(
                "One or more requested features could not be installed: "
                "Recharge={}, NoMagicBoost={}",
                rechargeInstalled,
                noMagicInstalled);
        }
        return rechargeInstalled && noMagicInstalled;
    }
}
