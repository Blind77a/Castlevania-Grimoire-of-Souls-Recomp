# GoSRecomp

Castlevania: Grimoire of Souls (Apple Arcade, macOS) running natively on Windows.
Same idea as [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp).

> You need your own copy of the game: the macOS app, version 1.4.2, as a `.dmg`.
> This repo has no game code or data. The releases have no game data (art,
> sound, text): the setup takes it from your own image. Their `GameAssembly.dll`
> does contain the game's code, machine-translated to x64, and it doesn't run
> without the original files from your image. Please don't ask for game files
> or post links to them. Not affiliated with Konami, Apple or Unity.

## Changes in this fork

This fork adds the following on top of [BLSNCVS/Castlevania-Grimoire-of-Souls-Recomp](https://github.com/BLSNCVS/Castlevania-Grimoire-of-Souls-Recomp) `main`.

- **Start opens the menu.** On the hub, Start opens the options menu, the same one as the Menu button. It stays off while a popup or the hub tutorial is up.
- **Start presses the button.** On screens where Start jumps to a button, like Start on the quest preparation screen, it now presses that button too, instead of only moving the cursor there. Greyed-out buttons stay unpressed.
- **Quit Game.** The in-game menu has a Quit Game button beside Return to Title. It saves, then asks with the title screen's confirm dialog before it quits.
- **Skip the startup logos.** `skip_intro=1` (the default) skips the Apple Arcade movie and the Konami logo. The title menu stays, and Continue is usable before the fade-in and the Castlevania logo shine. `skip_intro=0` plays the logos.
- **Xbox button icons.** On Windows 10 and 11, Xbox pads show Xbox icons. The game's check was the macOS version test, so those pads used to show generic Button labels.
- **Missing effects.** The dash, dodge and slide dust, enemy flame deaths and background flames show up again. Wall candles and torches keep their fire color and draw in front of the wall, and brazier flames sit in front of their stands.
- **Chest glow and coins.** Opening a chest shows its golden flare again, and the coins dropped by defeated enemies render and bounce on screen.
- **Falling rain.** Rainy stages such as level 1-3 in Chapter III show the rain coming down again, not only the splashes on the ground.
- **Snappier controls (optional).** Two new `[display]` settings in `gosr.ini`: `frame_queue` sets how many frames the game prepares ahead of the screen, and `refresh` sets the monitor's refresh rate in exclusive fullscreen. Both are off unless you set them. See [Most responsive controls](#most-responsive-controls).
- **Debug menu in the game.** F8, or clicking both sticks (L3 + R3), opens the debug menu as one of the game's own dialogs: its frame, title bar, buttons, font and sounds, on top of any screen, also in exclusive fullscreen. It starts on a main page of sections, works with the controller (D-pad to move, A to use, B to go back, LB / RB to change section) or the mouse, and pauses the game while it is open. Every change to your save asks first and warns that it cannot be undone and can spoil the intended experience. Give item now goes by category, then item, then amount, and the gold and gem counts update right away. The old separate window is still there with `window=1` under `[debug]`.
- **Save backup and restore.** The debug menu's Save backup section copies your save into a dated folder beside the save folder (`Castlevania_ Grimoire of Souls - GoSRecomp backups`) and lists the backups to restore. Restoring backs up your current save first, then restarts the game with the restored one.
- **Auto input.** The debug menu's Auto input section makes attack, jump or sub-weapon repeat while you hold the button in battle, at 6, 10 or 15 presses a second. Characters that charge an attack by holding the button, Maria for one, repeat instead of charging while it is on. The choice is kept in `gosr.ini` (`auto_attack`, `auto_jump`, `auto_subweapon`, `auto_speed` under `[controller]`).
- **Settings in the debug menu.** A Settings section switches the window mode, refresh rate, V-Sync, frame queue, startup logos, d-pad movement and the backup at start, without editing `gosr.ini`. Restart now applies them: it writes your save first, then restarts the game (not during a battle).
- **Automatic save backups.** Each start backs up your save into the backups folder, marked "auto". The newest 10 automatic backups are kept; your own are never removed. `auto_backup=0` under `[game]` turns it off.
- **Achievements.** The game's 12 Game Center achievements (the tutorial, Chapters I to X and every challenge) work offline. They unlock as the game decides them, and also when a chapter's closing quest is cleared without its story scene marked as seen. An unlock shows a banner at the top of the screen in the game's style. The menu's own Achievements button, in its User Information section, which only showed with Game Center, is back at the top of the menu and lists them, greyed out until unlocked, with how to get each one. They are kept in `Castlevania_ Grimoire of Souls - GoSRecomp achievements.ini` beside the save folder.

## What's in it

- Fully offline. The game's server is emulated inside the game, so there's no
  account and no network
- Direct3D 11, with the post-processing and the SPFX particle effects
- Xbox and PlayStation controllers, keyboard and mouse. Mouse buttons can be
  bound in the game's own keyboard options
- Frame rate cap, V-Sync, fullscreen / borderless / windowed and resolution in
  `gosr.ini`
- Every language the game shipped with
- A debug menu in the game, on F8 or L3 + R3, styled like the game's own
  dialogs and driven by the controller: gold, gems, every item by name, unlock
  everything, open every quest and banner, skip the tutorial, invincibility,
  damage multiplier, save backup and restore, auto attack / jump /
  sub-weapon, and the display and controller settings. It asks before every
  change to your save, because it changes it for good and skips the game's
  progression
- Achievements, offline: the game's 12, in its own menu, with an unlock banner
- An automatic backup of your save at every start (the newest 10 are kept)

## Installing

1. On the Releases page, download `GoSRecomp-x.y.z.zip` (not "Source code",
   that's just this repo) and extract it into a new, empty folder.
2. Drag your `.dmg` onto `Setup.bat`, or run it and paste the path. It takes a
   few minutes and needs about 3 GB free while it runs. If your image isn't
   version 1.4.2 it stops and tells you.
3. Start the game with `Play.bat`.

To update, extract the new release into a new folder and run `Setup.bat`
again. Your save stays where it is:
`%USERPROFILE%\AppData\LocalLow\Konami Digital Entertainment Co_, Ltd_\Castlevania_ Grimoire of Souls`

Needs Windows 10 or 11, 64-bit, and a Direct3D 11 GPU.

## Settings

Everything is in `gosr.ini` next to `GoS.exe`, and each line is explained in
there. Leave a value empty to keep the game's own behavior.

| Section | What |
|---|---|
| `[game]` | `language` for the very first start (empty follows your Windows language); after that, the game's options decide. `skip_intro=1` skips the Apple Arcade movie and the Konami logo; the title menu stays, and Continue is usable before the fade-in and logo shine. `auto_backup=0` stops the save backup at each start (the newest 10 are kept) |
| `[display]` | `fps` (0 = no cap), `vsync` (empty: only where it gives 60 fps), `mode` (fullscreen, borderless, windowed), `width`, `height`, `refresh` (monitor Hz in exclusive fullscreen; empty: your desktop's rate), `frame_queue` (1-3 frames prepared ahead; empty: Unity's own, 2) |
| `[controller]` | `layout=pc` (default), or `mac` for the tables the macOS build used. `dpad_move=0` stops the d-pad from moving your character. `xinput=0` if an Xbox controller misbehaves. `standard`: other controllers read by their standard buttons (1, default), for every controller (2) or off (0). `auto_attack`, `auto_jump`, `auto_subweapon` (1 = holding the button repeats it in battle), `auto_speed` (6, 10 or 15 presses a second) |
| `[effects]` | `flip_fix=1`: experimental fix for effects drawn upside down in some screens (off by default) |
| `[mouse]` | `buttons=0` turns off mouse buttons in the keyboard options |
| `[debug]` | `menu=0` turns off the debug menu, `hotkey` changes F8 (L3 + R3 always opens it), `window=1` brings back the old separate window |

Bindings are set in the game's own Gamepad and Keyboard screens. Mouse buttons
show up there as Mouse L / R / M / 4 / 5. If you customized your controller with
an older build, press Default once.

### Most responsive controls

These settings are for Video Game Esoterica, and for anyone else who wants
the controls as tight as they can get.

By default the game waits for the screen (V-Sync) and lets Unity prepare 2
frames ahead, which can add up to about 33 ms between a button press and seeing
it. For the snappiest combat, set these under `[display]`:

| Setting | What it does |
|---|---|
| `mode=fullscreen` | Exclusive fullscreen skips the Windows desktop compositor. Needed for `refresh`. |
| `refresh=60` | Runs the monitor at 60 Hz while the game is open (Windows puts your usual rate back when it closes). The game runs at 60 fps, so this paces frames evenly; on a 120 Hz screen it otherwise waits for every 2nd refresh. |
| `vsync=0` | Doesn't wait for the screen. Lowest delay, but a tear line can show when the picture moves. |
| `frame_queue=1` | Prepares only 1 frame ahead instead of 2: a press shows up to a frame (~16 ms) sooner. |

All four together give the lowest delay. If walking through a level hitches
or stutters, set `vsync=1` first (no tearing, still low delay at 60 Hz), then
`frame_queue=2`. Leave `fps` empty: the game was made for 60 fps.

## Status

It plays. The story, quests, summons, roster and upgrades work, and so do the
effects. Things I know about:

- An effect can show up a few frames late the first time you see it while its
  shaders get converted. After that they're cached.
- The game was made for 60 fps. Higher caps work, but if something feels too
  fast, go back to 60.

### Troubleshooting

- **Setup.bat says "not a GoSRecomp release"**: you downloaded "Source code". Download `GoSRecomp-x.y.z.zip` from the Releases page.
- **Setup.bat says the files are not the expected ones**: the `.dmg` must be the macOS app, version 1.4.2, unmodified.
- **Characters T-pose in the hub or roster**: fixed in 1.0.9. If you still see it, send a crash report.
- **Pink screen in Chapter IX, levels 2-2 to 2-5**: fixed in 1.1.0. Extract the new release and run `Setup.bat` again, the fix is in the converted data.
- **Attacks don't hit, enemy attacks are invisible, Maria walks oddly**: the game must run at 60 fps. 1.0.8 does that by default; if you changed `fps` or `vsync`, set `fps=60` and `vsync=0`.
- **"An error happened when reading the video"**: Windows can't decode the game's videos. Update your graphics driver; on Linux use GE-Proton.
- **The language in `gosr.ini` does nothing**: it only applies on the very first start. Change it in the game's options.
- **Controller buttons or sticks are wrong**: PlayStation and Xbox controllers, press Default once in the game's controller options. Other controllers (8BitDo, Switch Pro, ...): use their X-input (Xbox) or Switch mode; if buttons are still wrong, set `standard=2` under `[controller]` in `gosr.ini`. An Xbox controller that misbehaves: `xinput=0`.
- **The d-pad moves your character** and you don't want it: `dpad_move=0`.

Found a bug? Start the game with `Play with crash log.bat`. When it closes it
makes a `crash_report.zip` with the logs, a copy of your save and settings, and
a dxdiag report. Open an issue and attach it. Have a look inside first if you
care about what's in there.

## Building

It builds on Linux and cross-compiles for Windows. You need the macOS game
files, Unity 2020.3.30f1 with Windows IL2CPP support and an MSVC sysroot from
xwin. [BUILDING.md](BUILDING.md) goes through every step. How it works is in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## AI Note

AI was used to help with the coding. Still, the build was tested on different machines, and the code was reviewed. 
Sorry if there are still problems. I do it for the love of the game.

## Repository layout

```
tools/recomp/        arm64 to C++ translator and the generators around it
tools/analysis/      Function tables and surveys of the game binary and data
tools/shaders/       Metal to HLSL shader conversion
tools/patch/         Binary patches the setup applies to your own files
recomp/runtime/      What the translated code runs on (registers, memory, exceptions)
recomp/winbuild/     GameAssembly.dll: runtime glue, debug menu, display, controls
recomp/server/       The offline server
plugins/spfx/        The particle effects plugin, translated, OpenGL drawn on D3D11
plugins/apple/       Stand-ins for the Apple-only plugins
packaging/windows/   Setup.bat, Play.bat, gosr.ini and the readme that ship
docs/                How it works, shaders, server notes
```

## Credits

- [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp), for
  showing what static recompilation can do for a game
- [Il2CppDumper](https://github.com/Perfare/Il2CppDumper),
  [LIEF](https://github.com/lief-project/LIEF),
  [Capstone](https://github.com/capstone-engine/capstone),
  [Unicorn](https://github.com/unicorn-engine/unicorn) and
  [UnityPy](https://github.com/K0lb3/UnityPy), for taking the game apart
- [glslang](https://github.com/KhronosGroup/glslang) and
  [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross), built into the
  effects plugin
- [7-Zip](https://www.7-zip.org/), used by the setup
- [xwin](https://github.com/Jake-Shadle/xwin), for building Windows binaries on Linux

Licenses are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Castlevania
is a trademark of Konami.
