# Half-Life Unified SDK

The [Half-Life Unified SDK](docs/README.md#developer-resources) is a project that provides an updated version of the Half-Life SDK, with full support for the expansion packs Opposing Force and Blue Shift as well as new features.

[![CI/CD](https://github.com/twhl-community/halflife-unified-sdk/actions/workflows/ci-cd.yml/badge.svg?branch=master)](https://github.com/twhl-community/halflife-unified-sdk/actions/workflows/ci-cd.yml)

The SDK provides a CMake-based project structure with support for Visual Studio 2019 and 2022 on Windows and GCC 11 on Linux, as well as many bug fixes and improvements.
Opposing Force and Blue Shift features have been integrated with code refactored to reduce the amount of code duplication.

**You will need a strong grasp of of C++, build systems like Visual Studio or Make (depending on the platform you're developing for), CMake, command line interfaces, version control systems (Git in particular) and package managers (vcpkg in particular) to make a mod with this SDK. If you do not have experience with these technologies then you will have a very hard time getting started.**

The goal of the Unified SDK is to allow modders to make mods based on these games, while providing bug fixes that could be applied to the official games as well in addition to bug fixes that would be a breaking change. Check out the documentation linked below for a list of features.

A mod installation is also provided to allow players to play these games with all bug fixes applied. It includes files that are required when making a mod based on this SDK.

The Unified SDK works on the `steam_legacy` and anniversary versions of the game but requires the game to be switched to the `steam_legacy` version during installation. See the installation guide for more information.

> <span style="background-color:orange; color: black">Warning
> </br>
> Due to how some SDK features are implemented demos will not work properly. Use an external program like OBS to record the game.</span>

> <span style="background-color:darkseagreen; color: black">
> Note
></br>
> Maps made for the original games need to be upgraded using the MapUpgrader tool before they can be played in this mod.</span>

The following types of changes are **in scope** for this project:
* Bug fixes
* Features to improve the game's code (refactoring, generalizing, simplifying). This does not include complete redesigns of systems as this makes it much harder for modders to integrate changes and get started with Half-Life modding
* Fixing game-breaking bugs in game assets (e.g. soft-locked trigger setups)

The following types of changes are **out of scope**:
* Graphical upgrades
* Physics engine changes
* Other engine changes
* Gameplay changes

If you need help setting up the SDK or developing a mod please ask on the [TWHL website](https://twhl.info/) or on its [Discord server](https://discord.gg/jEw8EqD).

The TWHL wiki has tutorials to guide you through making a mod: https://twhl.info/wiki/page/Half-Life_Programming_-_Getting_Started

See the `#welcome` channel for more information about the Discord server. Please do not use the `#unified-sdk` channel for general help requests, there are channels for modding help.

See the TWHL thread for status updates about these projects: https://twhl.info/thread/view/20055

Updated game assets provided by [Marc-Antoine Lortie](https://github.com/malortie).

The source code for the C# tools can be found [here](docs/README.md#developer-resources)

The map decompiler can be found [here](docs/README.md#developer-resources)

# Documentation

See [docs/README.md](docs/README.md) for a list of documentation, resources and tutorials.

See [FULL_UPDATED_CHANGELOG.md](FULL_UPDATED_CHANGELOG.md) for changes inherited from Half-Life Updated and [UNIFIED_CHANGELOG.md](UNIFIED_CHANGELOG.md) for changes specific to the Unified SDK.

## Documentation format

All of the SDK's documentation is written in Markdown. Github allows you to view these files as well as navigate through them.

For offline use the best way to view the documentation is to use Visual Studio Code: https://code.visualstudio.com

Visual Studio Code includes Markdown preview using the keyboard shortcuts `CTRL+SHIFT+V` (switch to preview) and `CTRL+K V` (open preview side by side; press `CTRL+K`, let go of both keys, then press `V`).

It is recommended to open the repository folder using `File->Open Folder...` to allow navigation to the files included in the repository.

Consult Visual Studio Code's documentation for more information about Markdown support: https://code.visualstudio.com/docs/languages/markdown

# Development process

This project uses terms commonly used in software and game development to label development builds. See this article for more information about the stages that a game's development goes through: https://en.wikipedia.org/wiki/Video_game_development#Milestones

The current milestone is listed in the title of a release and is also visible in-game in the project info overlay.

The repository currently contains a pre-alpha version in the master branch. This is because there is no stable version yet. Once version 1.0.0 has been released the master branch will contain stable releases and a development branch will be created to track the development version.

> **Note** **For modders**
> Pre-alpha builds are not ready to be used to develop mods. Features are still being developed and are unfinished and buggy.

> **Note** **For players and testers**
> During pre-alpha development testing is limited to individual features if they are noted to be ready for this.

# Requirements to run mods built with this SDK

Only the latest Steam version of Half-Life is supported. For the Opposing Force and Blue Shift campaigns you will need to own the games and have them installed to install and play their maps.

All other assets are permitted by Valve to be used in mods and are included in the mod installation.

The system requirements are mostly the same as Half-Life itself, but for Windows users you will need Windows 7 or newer to run it.

The Half-Life Unified SDK mod installation is currently available only on Windows.

# Building this SDK

See [DEVELOPING.md](DEVELOPING.md) and [BUILDING.md](BUILDING.md)

# Mod installation instructions

See [INSTALL.md](INSTALL.md)

# What isn't supported

Backwards compatibility with WON and older versions of Steam Half-Life is not supported. Xash isn't supported. You cannot use the Unified SDK's client to play on vanilla servers, you also cannot use vanilla clients to play on Unified SDK servers.

Placing Unified SDK game dlls in vanilla installations is not supported.

Metamod and AMXMod are not supported. The changes made to the source code render it incompatible with various parts of their hooking systems.

# Deathmatch Classic and Ricochet

The source code for Deathmatch Classic and Ricochet is in the original Half-Life SDK. The purpose of these updated repositories is to provide updated versions only for Half-Life and its expansion packs, so the source code for these mods has been removed.

Since the vanilla versions don't compile under newer versions of Visual Studio separate repositories have been made that provide the same updates to make them compile. These projects can be found [here](docs/README.md#other-source-code-repositories).

Unlike the other updated repositories these only provide basic fixes. No further development and support will be provided.

# Half Life 1 SDK LICENSE

Half Life 1 SDK Copyright© Valve Corp.  

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE CORPORATION (“Valve”).  PLEASE READ IT BEFORE DOWNLOADING OR USING THE HALF LIFE 1 SDK (“SDK”). BY DOWNLOADING AND/OR USING THE SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.

You may, free of charge, download and use the SDK to develop a modified Valve game running on the Half-Life engine.  You may distribute your modified Valve game in source and object code form, but only for free. Terms of use for Valve games are found in the Steam Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/ 

You may copy, modify, and distribute the SDK and any modifications you make to the SDK in source and object code form, but only for free.  Any distribution of this SDK must include this license.txt and third_party_licenses.txt.  
 
Any distribution of the SDK or a substantial portion of the SDK must include the above copyright notice and the following: 

DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED “AS IS”.  VALVE AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, TITLE AND FITNESS FOR A PARTICULAR PURPOSE.  

LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 
 
If you would like to use the SDK for a commercial purpose, please contact Valve at sourceengine@valvesoftware.com.


# Half-Life 1

This is the README for the Half-Life 1 engine and its associated games.

Please use this repository to report bugs and feature requests for Half-Life 1 related products.

## Reporting Issues

If you encounter an issue while using Half-Life 1 games, first search the [issue list](https://github.com/ValveSoftware/halflife/issues) to see if it has already been reported. Include closed issues in your search.

If it has not been reported, create a new issue with at least the following information:

- a short, descriptive title;
- a detailed description of the issue, including any output from the command line;
- steps for reproducing the issue;
- your system information.\*; and
- the `version` output from the in‐game console.

Please place logs either in a code block (press `M` in your browser for a GFM cheat sheet) or a [gist](https://gist.github.com).

\* The preferred and easiest way to get this information is from Steam's Hardware Information viewer from the menu (`Help -> System Information`). Once your information appears: right-click within the dialog, choose `Select All`, right-click again, and then choose `Copy`. Paste this information into your report, preferably in a code block.

## Conduct


There are basic rules of conduct that should be followed at all times by everyone participating in the discussions.  While this is generally a relaxed environment, please remember the following:

- Do not insult, harass, or demean anyone.
- Do not intentionally multi-post an issue.
- Do not use ALL CAPS when creating an issue report.
- Do not repeatedly update an open issue remarking that the issue persists.

Remember: Just because the issue you reported was reported here does not mean that it is an issue with Half-Life.  As well, should your issue not be resolved immediately, it does not mean that a resolution is not being researched or tested.  Patience is always appreciated.

# Contributors

This is a list of everybody who contributed to these projects. Thanks for helping to make them better!

If you believe your name should be on this list make sure to let us know!

* Sam Vanheer
* JoelTroch
* malortie
* dtugend
* Revenant100
* fel1x-developer
* LogicAndTrick
* FreeSlave
* zpl-zak
* edgarbarney
* Toodles2You
* Jengerer
* thefoofighter
* Maxxiii
* johndrinkwater
* anchurcn
* DanielOaks
* MegaBrutal
* suXinjke
* IntriguingTiles
* Oxofemple
* YaLTeR
* Ronin4862
* the man
* vasiavasiavasia95
* NongBenz
* Hezus
* Anton
* ArroganceJustified
* a1batross
* zaklaus
* Uncle Mike
* Bacontsu
* L453rh4wk
* P38TaKjYzY
* hammermaps
* LuckNukeHunter99
* Veinhelm
* jay!
* BryanHaley
* λλλλλλ
* Streit
* rbar1um43
* LambdaLuke87
* almix
* sabian

## Special Thanks

* Valve Software
* Gearbox Software
* Alfred Reynolds
* mikela-valve
* TWHL Community
* Knockout
* Gamebanana
* ModDB
