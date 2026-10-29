# Wii Party U - Highway Rollers Board Randomizer
A mod inspired by the [Wii Party equivalent](https://gamebanana.com/mods/645316) that Poofesure has been playing.

> [!NOTE]
> This mod is still in development, if you have any problems, please create an [issue](https://github.com/luamaxxing/Highway-Rollers-Randomizer/issues)!

# Compatability
- This mod is made for modded Wii U's using [Aroma](https://aroma.foryour.cafe/) custom firmware, on version **Beta-27** or higher.
- All regions are supported (`NTSC-U`, `PAL`, & `NTSC-J`) as long as you are on **v0.1.3** of the mod or higher.
    - `NTSC-U` and `PAL` have been tested on actual hardware, but `NTSC-J` hasn't been. Create an [issue](https://github.com/luamaxxing/Highway-Rollers-Randomizer/issues) if you encounter any problems.

# Installation
1. Setup [Aroma](https://wiiu.hacks.guide/aroma/getting-started.html) or update it through **Aroma Updater**.
2. Get the latest release from [releases](https://github.com/luamaxxing/Highway-Rollers-Randomizer/releases).
3. Copy `HighwayRollersRandomizer.wps` into `sd:/wiiu/environments/aroma/plugins`.

## Dependencies
These should all already be installed with Aroma.
- Requires [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/aroma/modules`.  
- Requires [FunctionPatcherModule](https://github.com/wiiu-env/FunctionPatcherModule) in `sd:/wiiu/environments/aroma/modules`.  
- Requires [NotificationModule](https://github.com/wiiu-env/NotificationModule) in `sd:/wiiu/environments/aroma/modules`.

# Usage Instructions
- By default, the randomizer is **off**.  
- To enable the randomizer, load into Wii Party U and press `L + D-Pad Down + Minus` to open the Aroma plugin menu.  
- Navigate to the plugin with your D-Pad, and press `A`, then press `A` while selecting `Randomizer Enabled` to set it to true.  
- Hit your home button to exit the plugin menu, and then exit and load back into Highway Rollers if you already loaded into a game.

## Settings
- `Randomization Method` (default: `Shuffle Existing`)  
    - Shuffle Existing: Shuffles the board while keeping the number of each space type unchanged
    - Randomize Independently: Each space is randomized individually
        - This leads to much more chaotic generation, since every space type has an equal chance to spawn
- `Re-Randomize After Minigames` (default: `false`)
    - Instead of randomizing once at the start of the game, the board will randomize after every single minigame (Including versus spaces)

# Development
## Windows (Requires Docker Desktop)
```powershell
# Release
.\build.ps1 release

# Debug
.\build.ps1 debug
```
## Linux
```bash
# Build docker image (only needed once)
docker build . -t highway-rollers-builder

# Release
docker run --rm -v "${PWD}:/project" highway-rollers-builder make -B DEBUG=0 TARGET=HighwayRollersRandomizer

# Debug
docker run --rm -v "${PWD}:/project" highway-rollers-builder make -B DEBUG=1 TARGET=HighwayRollersRandomizer_DEBUG
```