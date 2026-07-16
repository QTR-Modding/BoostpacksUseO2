# Implementation research and decision record

## 0.4.1 breathing-reserve safeguard

Paid boost recharge uses only
`max(0, current Oxygen - permanent Oxygen * configured fraction)` as
available funding. Starfield 1.16.244's native `GetValuePercent` evaluator
and Papyrus `ObjectReference.GetValuePercentage` wrapper both call
ActorValueOwner slot 2 (`GetPermanentActorValue`) for the denominator and
slot 1 (`GetActorValue`) for the numerator. The reserve therefore uses the
engine's own live O2-capacity definition, including peak-value modifiers,
rather than a guessed base or fixed maximum.

Current and permanent O2 are read both before fuel restoration and again
immediately before payment. A transaction is partially capped at the
percentage boundary and stops rather than intentionally debiting below it.

The DLL does not restore O2 after payment. A post-debit correction could not
distinguish this transaction from a simultaneous hazard, sprint, or breathing
drain and might refund unrelated survival costs. The reserve therefore limits
only the debit requested by boost recharge; other systems remain free to lower
O2 through or below the threshold. Strict accounting still removes fuel that
was not actually paid for.

The single reserve setting defaults to 20%. This sits above Real O2's
documented 15% health-damage onset while leaving 80% of each pack-dependent O2
capacity available for paid recharge. Any finite value at or below zero
disables the safeguard, values above 100 clamp to 100, and invalid input is
ignored rather than silently disabling protection.

## 0.4.0 atmosphere and no-magic extension

### Main-thread atmosphere cache

Real O2 uses vanilla `ENV_CND_InSealedOrBreathableLocation [002FAA06]`.
Its exact meaning is a sealed environment or an O2/LowO2/HighO2 atmosphere;
CO2, H2, methane, nitrogen, and vacuum do not qualify. Weather hazards are
separate from breathability and are intentionally not mixed into this rule.

Starfield 1.16.244's native `ConditionForm.IsTrue` wrapper is Address Library
ID 114781. It correctly constructs the runtime 0x70-byte condition parameter
block and evaluates the full AND/OR list. CommonLibSF revision 84f8589 has
stale layouts here: `ConditionCheckParams` is declared as 0x68, and
`BGSConditionForm.conditions` is labeled at +0x30 rather than runtime +0x38.
The DLL therefore calls the engine wrapper without dereferencing either stale
layout.

The native is marked not callable from tasklet threads. A permanent SFSE
main-thread task evaluates it once per frame and atomically publishes the
answer. The ValueModifierEffect worker hook only reads that boolean and
defaults to charging O2 until a valid in-world result exists.

### Hold-compatible no-magic boost

Magic Boost Disabler 0.9 uses a hidden ability to set the player's
`BoostpackMinFuelRequired [0035524C]` to 200 while
`ActorShouldShowSpacesuit [00194ABF]` is false. The method blocks ordinary
activation, but Starfield's `bUsePressAndHoldControls=1` path treats the
instantaneous threshold as zero and bypasses it.

Reverse engineering confirmed that hold mode also bypasses the minimum-fuel
threshold on the initial activation: with the INI true, the gate tests only
`BoostpackFuel > 0`. Virtualizing the minimum actor value would therefore
still permit a first pulse and was rejected.

The replacement blocks the boost request itself without writing an actor value:

- JetpackEffect's `PlayerJumpPressEvent` sink vtable is Address Library ID
  448594, slot 0x01. When the suit condition is false, only that listener
  returns `kContinue` without entering the jetpack state machine.
- The event dispatcher continues to ordinary ground-jump listeners, so hidden
  ground jumping and its hold behavior remain native.
- JumpHandler vtable ID 433587, slot 0x08, is used only to release an already
  active boost if visibility changes to hidden. It requires
  `BoostpackActive [0002466E] > 0.5`, then routes through vanilla's release
  branch and suppresses repeats until physical release.
- The exact vanilla suit condition is evaluated on the input/game thread.

This is player-only and creates no plugin override, Papyrus state, actor-value
write, save data, or grounded-state guess.

## 0.3.0 live-test correction

The first live test crashed on the first payable recharge at
`Starfield.exe+0x1977526`, called from the O2 debit in `RechargeHook::Apply`.
The compiled three-argument `ModActorValue` expression called ActorValueOwner
vtable slot `0x06` but did not populate that slot's required fifth/source stack
argument. The engine interpreted stale stack data as a `TESObjectREFR*`.

Version 0.3.1 bypasses the ambiguous CommonLibSF overloads and explicitly calls
the runtime's no-source ActorValueOwner slot `0x07`. This is the same slot used
by Starfield's native queued O2-cost path. Binary verification must confirm that
each synthetic mutation calls vtable byte offset `0x38`, never the source-aware
slot `0x06` with an uninitialized fifth argument. The ValueModifierEffect
vtable, slot `0x1E`, callback ABI, form filtering, and delta direction were
independently reconfirmed by the crash trace and disassembly.

## Exact mechanic

- Native thrust consumes the native `BoostpackFuel` bar exactly as Starfield
  decides, including `bUsePressAndHoldControls=1`.
- Positive bar regeneration is no longer free.
- Each point actually restored consumes configurable player O2.
- Paid regeneration stops at the configured O2 reserve; with protection
  disabled, it stops at zero.
- Pack type, perks, thrust, fuel consumption, and boost physics stay native.
  Version 0.4.x adds only the documented reserve, free-recharge environment
  rule, and suit-hidden Jump input gate.
- Real O2's separate **Boost Uses O2** option is disabled.

## Engine facts verified for Starfield 1.16.244

- `BoostpackFuel` is AVIF [00024021]; `Oxygen` is AVIF [000002D5].
- Basic, Balanced, Power, and Skip packs use the same
  `RegenerateBoostpackFuel` MGEF [0002466C], with twelve conditioned rate
  branches across their four ENCH records.
- ValueModifierEffect vtable ID 448841 resolves to RVA 0x4CA7980. Slot 0x1E
  points to RVA 0x17857B0 in the installed executable.
- The normal constant-effect update at RVA 0x1785310 passes positive
  magnitude-times-delta-time to slot 0x1E.
- The normal slot 0x1E path calls ActorValueOwner modifier `kDamage` with that
  positive delta, restoring fuel.
- `GetActorValue(Oxygen)` is positive while O2 remains and reaches zero at
  exhaustion. The player update at RVA 0x1A2D8D4 checks the transition from
  positive to non-positive.
- Native O2 costs negate a positive cost before actor-value modification at
  RVA 0x1A63F3E–0x1A63F5F. Therefore O2 debit and fuel rollback use negative
  `kDamage`; positive `kDamage` restores.

## Route matrix

| Route | Coverage | Exactness | Cost/conflicts | Decision |
|---|---|---|---|---|
| INI/game settings only | Input/thrust tuning only | Cannot couple two actor values | Very low | Insufficient |
| Parallel fuel/O2 MGEFs on four ENCHs | Native packs and mods reusing those ENCHs | Same rate mid-bar; separate effects can overgrant, overdraw, or apply in different order at boundaries | Overrides four shared ENCHs; custom ENCHs need patches | Data-only fallback only |
| Replace shared recharge MGEF | Packs reusing it | A single normal MGEF changes one actor value only | Global shared-record conflict | Insufficient alone |
| Dual Value Modifier MGEF | Potentially broad | Primary and secondary are applied sequentially using requested magnitude; it does not charge actual clamped fuel gain. Negative second weight is also unproven in shipped Starfield records | Shared MGEF override | Approximation, not atomic |
| Perk entry points | Boost activation/delivery | No recharge-delta/two-AV exchange entry point | Record conflicts | Insufficient |
| Magic effect while `BoostpackActive` | Any thrust | Charges thrust time, not bar regeneration | Requires delivery/controller records | Solves the earlier hold-drain idea, not this goal |
| Papyrus `OnActorValueChanged` | Any pack changing the AV | Post-change event has no delta; one-shot registration and VM delay/coalescing create gaps and HUD flicker | QUST/alias/PEX/save state | Best Papyrus fallback, not exact |
| Papyrus timer polling | Any pack | Observes net changes only; boost use can hide simultaneous recharge | Continuous VM work/save state | Rejected |
| Rebuild recharge in Papyrus | Only reconstructed rates | Separate timed calls; loses arbitrary native/modded rates | Heavy controller and record burden | Rejected |
| Native game-loop polling | Any pack | Can claw back after a sample, not at the original transaction | Periodic work and visible races | Rejected |
| Native ActorValueChanged event | Any pack | Notification is after the value changed | Event plumbing | Rejected as transfer core |
| Hook Player ActorValueOwner slot 06 | Every positive fuel mutation | Synchronous, but also catches transient/internal/script restores and balancing legs unrelated to natural recharge | Broader compatibility surface | Technically viable, too broad |
| Hook all ActorValueOwner implementations | Player and NPCs | Synchronous | Global hook; NPC O2 is not a Real O2 survival resource | Rejected |
| Patch the normal recharge callsite | Vanilla path | Exact | Runtime-specific code patch and narrower compatibility | Viable, inferior to the vtable filter |
| Hook ValueModifierEffect slot 0x1E and filter player + positive BoostpackFuel | Native packs and any ordinary ValueModifier recharge MGEF | Synchronous; caps before restore and charges actual gain | One runtime-specific chained vtable hook; no records/save state | Selected |
| Manual O2 burner/hotkey | User-triggered refill | Can be exact per command but is not automatic | New input/UI dependency | Different mechanic |
| Replace boost fuel with O2 per activation | Any supported activation | Bypasses native bar accounting | Conflicts with desired behavior | Real O2's optional model; rejected for this goal |
| Separate physical boost fuel/canisters | Pack-specific survival system | Exact within its own system | Broad overhaul | Different mechanic |

## Existing mods checked

- **Excursion – Zero Atmosphere Survival** says its boost pack draws on O2
  reserves instead of infinitely regenerating fuel. It is a paid, broad
  survival Creation and explicitly overlaps/incompatibly with Real O2. Its
  internal X-for-X accounting is not publicly verifiable.
  <https://creations.bethesda.net/en/starfield/details/675ff5a6-da1c-4e67-87dd-30e8d022a5dc/Excursion_Zero_Atmosphere_Survival>
- **Boostpack to Jetpack** has an O2 burner that refills fuel only when the
  player presses B; it is not automatic.
  <https://www.nexusmods.com/starfield/mods/3800>
- **Real O2** optionally makes boost activation use O2 instead of boost fuel;
  it does not make native recharge consume O2.
  <https://www.nexusmods.com/starfield/mods/12303>
- **Starvival** replaces infinite use with a separate boost-fuel/canister and
  refueling-station system.
  <https://www.nexusmods.com/starfield/mods/6890>
- **Infinite Boost Fuel** periodically restores fuel; it demonstrates polling,
  not O2-funded recharge.
  <https://github.com/Meridiano/StarfieldDLL/tree/main/InfiniteBoostFuel>

No narrow, freely available mod matching this exact automatic exchange was
found in the Nexus, Bethesda Creations, and GitHub searches performed for this
project.

## Player-only scope

The exchange is intentionally filtered to the player. Real O2 simulates a
survival O2 reserve for the player, not NPCs. Applying the same cost to NPCs
would not create fair symmetry; it would make their boost behavior depend on
an actor value their AI/survival systems do not maintain. Native pack physics,
fuel consumption, and recharge remain untouched for NPCs.

## Rejected prototypes

Earlier prototypes with reversed O2/recharge sign assumptions were discarded.
They are not part of this repository or any release.
