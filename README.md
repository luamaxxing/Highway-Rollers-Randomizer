# Wii Party U - Highway Rollers Board Randomizer
A mod inspired by the [Wii Party equivalent](https://gamebanana.com/mods/645316) that Poofesure has been playing.

> [!WARNING]
> This mod only works for the North America (NTSC-U) version of the game.

# Compatability
- This mod is made for modded Wii U's using [Aroma](https://aroma.foryour.cafe/) custom firmware, on version **Beta-27** or higher.
- Don't have Aroma? Follow their [guide](https://wiiu.hacks.guide/aroma/getting-started.html).
- If you are on a different region Wii U, you can install the NTSC-U version of the game and still be able to load it as long as you have the [RegionFreePlugin](https://github.com/wiiu-env/region_free_plugin) installed.

# Installation
1. Get the latest release from [releases](https://github.com/luamaxxing/Highway-Rollers-Randomizer/releases).
2. Copy `HighwayRollersRandomizer.wps` into `sd:/wiiu/environments/aroma/plugins`.  

## Dependencies
These should all already be installed with Aroma, but if just make sure you have them installed if you get any errors related to them.
- Requires [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/aroma/modules`.  
- Requires [FunctionPatcherModule](https://github.com/wiiu-env/FunctionPatcherModule) in `sd:/wiiu/environments/aroma/modules`.  
- Requires [NotificationModule](https://github.com/wiiu-env/NotificationModule) in `sd:/wiiu/environments/aroma/modules`.

# Usage Instructions
- By default, the randomizer is **off**.  
- To enable the randomizer, load into Wii Party U and press `L + D-Pad Down + Minus` to open the Aroma plugin menu.  
- Navigate to the plugin with your D-Pad, and press `A`, then press `A` while selecting `Randomizer Enabled` to set it to true.  
- Hit your home button to exit the plugin menu, and then exit and load back into Highway Rollers if you already loaded into a game.

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