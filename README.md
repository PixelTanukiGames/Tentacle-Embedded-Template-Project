

# Tentacle Embedded Template Project

Template made for [Backseating Allowed Jam 2026](https://itch.io/jam/backseating-allowed-jam-2026)

An Unreal Engine 5.8 starter project with the Tentacle plugin (v1.2) already installed and set up. It gives you a working base for a game that reacts to Twitch and Kick: chat, follows, subs, raids, channel point redemptions, viewer properties and more.
The project includes sing-in to chat buttons, and chat log. Other functions are added by using the Tentacle events, as noted below.

The project is built on Unreal's First Person template and includes the Unreal intro/demo content.

## Tentacle Docs

- [Tentacle Docs (official)](https://www.tentacle.live/docs/1.2.0-alpha/unreal/)
- [Setting up Tentacle from scratch](https://www.tentacle.live/docs/1.2.0-alpha/unreal/quickstart)

Use this template if you're starting a new game and want Tentacle working right away. Follow the from-scratch guide if you already have a project and want to add the Tentacle plugin to it yourself. 

Tentacle **does** work with other versions and engines, see [Tentacle Official](https://www.tentacle.live/) for more!

## Requirements

- Unreal Engine 5.8
- Windows 64-bit, or macOS (Mac supports the core Tentacle module only; video transport is Windows only)
- Visual Studio 2022 with the "Game development with C++" workload on Windows, or Xcode on Mac. The plugin ships as source code, so Unreal compiles it the first time you open the project.
- For Paired mode: the Tentacle app installed and signed in on the same computer
- For Embedded mode: a Twitch and/or Kick account to sign in with in-game

## Getting Started

1. Clone the repo:
   ```
   git clone https://github.com/PixelTanukiGames/Tentacle-Embedded-Template-Project.git
   ```
2. Open `TentacleTemplateProj/TentacleTemplateProj.uproject`. When Unreal asks to rebuild missing modules, click Yes.
3. Open the Tentacle level: `Content/TentacleTemplate/L_TentacleTemplateLevel`. The project starts in the Unreal intro room by default.
4. Select the Tentacle actor in the level and choose a mode (see below).
5. Press Play.

## Project Layout

```
TentacleTemplateProj/
  TentacleTemplateProj.uproject
  Config/                         Project settings
  Content/
    TentacleTemplate/             Tentacle starter content
      L_TentacleTemplateLevel     Main Tentacle level
      BP_TentacleManager_GI       Game Instance that manages Tentacle
      GM_Tentacle                 Tentacle game mode
      WBP_TentacleWidget          Tentacle UI widget
      WBP_ChatMessage             Chat message widget
    FirstPerson/                  First person character, game mode, controller
    Characters/Mannequins/        Manny and Quinn meshes and animations
    Input/                        Enhanced Input actions and touch controls
    LevelPrototyping/             Grid materials and blockout meshes
    DemoTemplate/                 Unreal intro room and demo content
  Plugins/
    Tentacle/                     Tentacle plugin v1.2
      Source/Tentacle/                Runtime: connection, events, chat, viewers, embedded core
      Source/TentacleVideoTransport/  Runtime (Windows only): Spout video send/receive
      Source/TentacleUncooked/        Editor only: Blueprint "Platform Request" node
      Content/                        Tentacle actor, guest blueprints, materials
      Binaries/ThirdParty/            Tentacle core library
      ThirdParty/                     Spout library and headers
```

## Paired vs. Embedded

The Tentacle actor has two settings: **Editor Mode** (used when you Play In Editor) and **Build Mode** (used in packaged and standalone builds). Each can be set to:

- **Paired**: the game connects to the Tentacle app running on the same computer. The streamer signs in through the app.
- **Embedded** (default): Tentacle runs inside the game, and the player signs in to Twitch or Kick in-game. No app needed. This is the one to use for games you ship to players.

You can override the mode from the command line, which is useful for testing a packaged build:

```
-tentacle-mode=paired
-tentacle-mode=embedded
```

### Embedded mode settings

These are on the Tentacle actor under **Tentacle > Embedded**.

| Setting | What it does |
| --- | --- |
| Embedded Data Directory | Where Tentacle saves its database, signed-in accounts and log. Empty uses `Saved/Tentacle`. |
| Twitch Client Id | Your own Twitch app's Client ID, so your game's name shows on the Twitch sign-in page. Empty uses Tentacle's. |
| Config Directory | Where your exported config files live. Default is `Config/Tentacle`. |

### Shipping your Tentacle config

1. In the Tentacle app, go to **Data > Export** and export your Timers, Custom Events, Viewer Properties and Bot commands.
2. Put the exported `.json` files in `TentacleTemplateProj/Config/Tentacle/`.
3. Files in that folder are packaged with the game automatically and applied every time Tentacle starts.

If you use a different folder, it has to be inside `Content/`, and you need to add it under **Project Settings > Packaging > Additional Non-Asset Directories to Package**.

## Troubleshooting

- **"Missing modules" or "could not be compiled":** make sure Visual Studio 2022 (or Xcode) and the Unreal C++ tools are installed.
- **Paired mode won't connect:** make sure the Tentacle app is open and signed in on the same computer.
- **Embedded mode does nothing:** check that `tentacle_ffi.dll` is in `Plugins/Tentacle/Binaries/ThirdParty/TentacleCore/Win64/`.
- **Player has to sign in every time:** set an Embedded Data Directory, or leave it empty to use the default `Saved/Tentacle`.

For anything else, see the [Tentacle Docs](TENTACLE_DOCS_URL).

## Credits

- Tentacle plugin by Eldritch One
- Template project by Pixel Tanuki Games
- Built on Unreal Engine's First Person template and starter content by Epic Games
- Video transport uses [Spout](https://spout.zeal.co/)
> Written with [StackEdit](https://stackedit.io/).

