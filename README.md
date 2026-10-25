# Quick Weapon Durability

A small TotK mod that puts remaining durability in the weapon quick menu, where the weapon name normally goes. Just the number, including any remaining fuse bonus.

Inventory names stay normal. Bows and shields are unchanged.

**TotK 1.2.1 only.** Tested in an existing modded Citron setup. Other versions and real hardware aren't tested.

## Install

Grab the zip from [Releases](https://github.com/yun-wulian/totk-quick-weapon-durability/releases), then put the `Quick Weapon Durability` folder in the game's mod folder. Enable it and restart the game.

This uses `exefs/subsdk6`, so it will conflict with another mod using that slot. It also needs an exlaunch-compatible `main.npdm` from your existing code-mod setup; this download doesn't include one. Keep the one you're already using with mods such as DFPS++.

## Build

You'll need Python 3 and [Zig 0.14.1](https://ziglang.org/download/0.14.1/). Put both on PATH, then run this in PowerShell:

```powershell
python -m pip install -r requirements.txt
.\Test.ps1
.\Build.ps1
```

The mod ends up in `dist/Quick Weapon Durability`. You can pass `-Python` and `-Zig` to `Build.ps1` if they aren't on PATH.

For the optional game-side checks, pass `-GameExeFs <folder>` with your own `main.nso`, `sdk.nso` and `rtld.nso` dump. Game files aren't included or needed just to build.

## Credits

Uses the exlaunch framework from [TOTK-DFPS, DFPS-1.1.0](https://github.com/MaxLastBreath/TOTK-DFPS/tree/DFPS-1.1.0), with a few Zig build fixes. Thanks to the exlaunch and DFPS contributors, and [dt-12345/totk_syms](https://github.com/dt-12345/totk_syms) for the symbol research.

GPL-2.0-only. Third-party notices are kept in the source.
