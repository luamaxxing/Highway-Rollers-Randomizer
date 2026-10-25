# Wii Party U - Highway Rollers Board Randomizer
A mod inspired by the [Wii Party equivalent](https://gamebanana.com/mods/645316) that Poofesure has been playing.

> [!WARNING]
> This mod only works for the North America (NTSC-U) version of the game.

# Compatability
- This mod is made for modded Wii U's using [Aroma](https://aroma.foryour.cafe/) custom firmware, on version **Beta-27** or higher.
- Don't have Aroma? Follow their [guide](https://wiiu.hacks.guide/aroma/getting-started.html).
- If you are on a different region Wii U, you can install the NTSC-U version of the game and still be able to load it as long as you have the [RegionFreePlugin](https://github.com/wiiu-env/region_free_plugin) installed.

# Installation
1. Copy `HighwayRollersRandomizer.wps` into `sd:/wiiu/environments/aroma/plugins`.
2. Requires [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/aroma/modules`.
3. Requires [FunctionPatcherModule](https://github.com/wiiu-env/FunctionPatcherModule) in `sd:/wiiu/environments/aroma/modules`.
4. Requires [NotificationModule](https://github.com/wiiu-env/NotificationModule) in `sd:/wiiu/environments/aroma/modules`.

# Usage Instructions
TODO

# Development
## Windows (Requires Docker Desktop)
```powershell
# Build all
.\build.ps1

# Build specific
.\build.ps1 [arg] # [ all | release | debug ]
```
## Linux
```bash
# Build docker image (only needed once)
docker build . -t highway-rollers-builder

# make 
docker run -it --rm -v ${PWD}:/project highway-rollers-builder make

# make clean
docker run -it --rm -v ${PWD}:/project highway-rollers-builder make clean
```