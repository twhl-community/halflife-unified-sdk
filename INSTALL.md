# Installing Half-Life Unified SDK

1. Download the latest version from the [Releases](docs/README.md#developer-resources) page.

2. Extract the archive to your Half-Life directory. It should place a directory called `hlu` in the `Half-Life` directory.

3. Install the Visual Studio 2022 re-distributable. You can find it in the `hlu/redist` directory.

4. Run the [ContentInstaller](docs/tools/content-installer.md) tool:
```bat
cd "path/to/Half-Life/directory"
dotnet "hlu/tools/ContentInstaller.dll" --mod-directory hlu
```

5. Restart Steam so the game is added to the list of games.

# Running the game

You can run the game by either running it through Steam or by launching Half-Life with the command-line parameter `-game hlu`.

# Project info text

You may see project info text overlaid on the screen while playing. See [Hud Project info](docs/features/hud-project-info.md) for more information.

# Pre-Alpha builds

Pre-Alpha builds can be downloaded from the [Github Actions](docs/README.md#developer-resources) artifacts section of each run.

These builds are available for up to 90 days after the run has finished.

Note that pre-alpha builds are work-in-progress builds and likely to be unstable and buggy.
