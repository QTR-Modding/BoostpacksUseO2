# Boostpacks Use O2

Boostpacks Use O2 is an SFSE plugin for Starfield 1.16.244 that makes positive
boostpack fuel recharge consume the player's O2. It preserves Starfield's
native boost fuel bar, thrust, pack types, perks, and tap or press-and-hold
controls.

Version 0.4.1 is the initial public release. The DLL and INI keep the internal
name `O2BoostRecharge` so upgrades cannot leave an older differently named DLL
loaded beside the new one.

## What it does

- Charges configurable O2 for each point of boost fuel actually restored.
- Stops paid recharge at a configurable percentage of live O2 capacity.
- Makes recharge free in sealed interiors and breathable atmospheres by default.
- Prevents boosting when vanilla hides the player's spacesuit.
- Supports `bUsePressAndHoldControls=1` without replacing native fuel use.
- Affects only the player. NPC boost behavior remains native.
- Adds no ESM/ESP/ESL, Papyrus scripts, load-order entry, or save data.

The default reserve is 20% of the player's current permanent O2 capacity, so it
scales with tanks, packs, perks, and O2 mods. The reserve protects only O2 spent
by this plugin; breathing, hazards, encumbrance, and other mods can still reduce
O2 below it.

## Requirements

- Starfield 1.16.244.0 on Steam
- [Starfield Script Extender 0.2.21](https://sfse.silverlock.org/)
- [Address Library for SFSE Plugins v22](https://www.nexusmods.com/starfield/mods/3256)

The native hooks and ABIs are runtime-specific. Windows Store and Game Pass
versions are not supported by SFSE.

## Installation

Install `BoostpacksUseO2-v0.4.1.zip` with a mod manager, or copy the archive's
`SFSE` folder into the game's `Data` folder. Launch Starfield through SFSE.

The installed files are:

```text
Data/SFSE/Plugins/O2BoostRecharge.dll
Data/SFSE/Plugins/O2BoostRecharge.ini
```

If [Real O2](https://www.nexusmods.com/starfield/mods/12303) is installed,
turn its separate **Boost Uses O2** option off. Otherwise both mods can charge
O2 for different parts of the same boost cycle.

If migrating from Magic Boost Disabler, first set its Gameplay Option to
**MAGICAL** so its perk becomes inactive and its actor-value write returns to
zero. This plugin provides the hold-compatible no-magic behavior.

## Configuration

Edit `Data/SFSE/Plugins/O2BoostRecharge.ini` before launching the game. Settings
are read once after game data loads; there is no hot reload.

| Setting | Default | Meaning |
|---|---:|---|
| `fO2PerFuel` | `1.0` | O2 spent per fuel point actually restored. Positive values clamp to `0.001` through `1000`. |
| `fMinimumO2ReservePercent` | `20.0` | Percentage of live permanent O2 capacity protected from this plugin. A finite value at or below zero disables the safeguard; values above 100 clamp to 100. |
| `bFreeInSealedOrBreathable` | `1` | Recharge is free in sealed interiors and O2, LowO2, or HighO2 atmospheres. |
| `bDisableBoostWhenSuitHidden` | `1` | Prevents boosting whenever vanilla says the player's spacesuit is hidden. Supports press-and-hold controls. |
| `bDebugLogging` | `0` | Troubleshooting only. Logs at most 120 recharge transactions per process. |

With free ambient recharge enabled, vacuum and non-breathable gases such as
CO2, H2, methane, and nitrogen still require stored O2. The check uses
Starfield's own sealed-or-breathable condition. Weather hazards remain a
separate system.

## Compatibility and limitations

- Verified with Basic, Balanced, Power, and Skip packs.
- Modded packs are covered when they use `BoostpackFuel` and an ordinary
  positive `ValueModifierEffect` recharge. Scripted or direct native actor-value
  refills can bypass the exchange.
- Other SFSE DLLs replacing the same `ValueModifierEffect`, `JetpackEffect`, or
  `JumpHandler` vtable entries may require compatibility work.
- The atmosphere cache initially fails closed, so recharge is charged until the
  first valid main-thread environment evaluation.
- A Bethesda runtime update requires a rebuilt and revalidated DLL.

## Uninstallation

Remove `O2BoostRecharge.dll` and `O2BoostRecharge.ini`. The plugin stores no save
data, so no clean-save procedure is required.

## Building

Clone recursively so the exact CommonLibSF and commonlib-shared revisions are
present:

```powershell
git clone --recursive https://github.com/QTR-Modding/BoostpacksUseO2.git
cd BoostpacksUseO2
xmake f -m releasedbg -a x64 -y
xmake -y
xmake run RechargeMathTests
```

The validated dependency revisions are:

- CommonLibSF `84f8589b15a6d588d28da16baa7459a1fc414ea7`
- commonlib-shared `40bdbcaf8fa691ee6daadc7b5cea8d43f794bef4`
- spdlog v1.16.0 (`486b55554f11c9cccc913e11a87085b2a91f706f`)

The build requires Xmake 3.0.8 or newer, MSVC with C++23 support, and the Windows
SDK. See [SOURCE.md](SOURCE.md) for corresponding-source details.

## License

Boostpacks Use O2 is licensed under GPL-3.0-or-later WITH the Modding Exception
AND GPL-3.0 Linking Exception (with Corresponding Source). See [COPYING](COPYING)
and [EXCEPTIONS](EXCEPTIONS).

## Credits

- Quant / QTR Modding
- CommonLibSF and commonlib-shared maintainers and contributors
- Ian Patterson and SFSE contributors
- meh321 for Address Library for SFSE Plugins
- rbtRvlt's Real O2 and Magic Boost Disabler as behavioral references

No files, code, or assets from Real O2 or Magic Boost Disabler are distributed.
