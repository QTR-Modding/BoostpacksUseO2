# Release verification manifest (v0.4.1)

- Public name: Boostpacks Use O2
- Internal plugin name: O2BoostRecharge
- Version: 0.4.1.0
- Target runtime: Starfield 1.16.244.0 (Steam)
- Required SFSE: 0.2.21
- Required Address Library: v22
- DLL SHA-256: `4D792FAA0CFF96492907F3AB5199F1D2F8FF9ABBDCA95B40BF4BE6799D493A52`
- Exports: `SFSEPlugin_Load`, `SFSEPlugin_Version`
- Recharge hook: `ValueModifierEffect` vtable ID 448841, slot 0x1E
- Environment evaluation: native `ConditionForm.IsTrue` ID 114781, main thread only
- No-magic start gate: `JetpackEffect` jump-press sink vtable ID 448594, slot 0x01
- Held-transition release: `JumpHandler` vtable ID 433587, slot 0x08
- Recharge boundary and strict-configuration tests: PASS
- Plugin records, Papyrus scripts, load-order entry, and save data: none

The final release hashes for all downloadable assets are published in the
attached `CHECKSUMS.txt` file.
