# ChangeWall

ChangeWall is a lightweight KDE Plasma wallpaper switcher for Wayland and X11. It changes wallpapers dynamically through Plasma DBus and wraps each update in a Qt Quick transition so the desktop feels instant instead of flickery.

## Architecture

- `src/profilemanager.*`: loads the wallpaper directory from JSON, persists the last wallpaper, and resolves sequential or random images from that folder.
- `src/shortcutmanager.*`: registers global shortcuts through `KGlobalAccel`.
- `src/plasmaintegration.*`: changes wallpapers using `org.kde.plasmashell /PlasmaShell evaluateScript`.
- `src/transitioncontroller.*` + `resources/qml/TransitionOverlay.qml`: creates one fullscreen transparent `QQuickView` per screen and performs a Hyprland-style transition with crossfade plus subtle zoom-in/out using `InOutQuad`.
- `src/commandserver.*`: exposes a small local socket so the same binary can run as a daemon and also accept CLI commands like `--next-wallpaper`.

## Why the transition is smooth

Plasma’s built-in wallpaper transitions are inconsistent across hardware and refresh rates. ChangeWall avoids that path entirely:

1. It creates a borderless, transparent fullscreen overlay.
2. The overlay draws the old wallpaper and the incoming wallpaper at the same time.
3. The new wallpaper opacity animates from `0.0` to `1.0` while the outgoing image slightly zooms out and the incoming one settles into place.
4. While the overlay is still visible, the real wallpaper is applied underneath through Plasma DBus.
5. The overlay disappears only after the real desktop has already switched.

That removes black frames and hides the timing variability of Plasma’s own wallpaper handoff.

## Build

Requirements on Arch Linux:

- `qt6-base`
- `qt6-declarative`
- `kglobalaccel`
- `plasma-workspace`
- `base-devel`

Build locally:

```bash
qmake6 changewall.pro
make -j"$(nproc)"
```

## Run

Start the daemon:

```bash
./changewall
```

On the first launch, ChangeWall opens a folder picker and writes:

```text
~/.config/changewall/config.json
```

Optional autostart on Plasma:

```bash
mkdir -p ~/.config/autostart
cp packaging/changewall.desktop ~/.config/autostart/changewall.desktop
```

Available commands:

```bash
./changewall --next-wallpaper
./changewall --random-wallpaper
./changewall --setup
./changewall --duration 550
```

Short forms also work:

```bash
./changewall next
./changewall random
./changewall setup
./changewall --next
./changewall --random
```

Default global shortcuts:

- `Meta+F11`: next wallpaper
- `Meta+F12`: random wallpaper

## Install

User-local install without packaging:

```bash
qmake6 changewall.pro
make -j"$(nproc)"
./scripts/install-user.sh
```

This copies:

- the binary to `~/.local/bin/changewall`
- helper commands to `~/.local/bin/changewall-next`, `~/.local/bin/changewall-random`, and `~/.local/bin/changewall-setup`
- an autostart entry to `~/.config/autostart/changewall.desktop`

At the first launch, the app asks for the wallpaper folder and creates the user config automatically.

System install:

```bash
qmake6 changewall.pro
make -j"$(nproc)"
sudo make install
```

That installs:

- `/usr/bin/changewall`
- `/usr/bin/changewall-next`
- `/usr/bin/changewall-random`
- `/usr/bin/changewall-setup`
- `/usr/share/applications/changewall.desktop`
- `/usr/share/changewall/config.json`
- `/etc/xdg/autostart/changewall-autostart.desktop`

Config format:

```json
{
  "wallpaperDir": "/home/villawolf/Imágenes/fondos"
}
```

Important:

- Global shortcuts are registered by `KGlobalAccel` when `changewall` starts for the first time.
- Package installation alone should not be expected to inject a GUI process into an already-running Plasma session; the intended flow is to launch `changewall` once, choose the folder, and keep using it.
- If you run `changewall --next-wallpaper` or `changewall --random-wallpaper` before the config exists, ChangeWall now opens the folder picker first and then applies the command.
- If the config file is missing, the first launch opens a directory picker instead of failing.

## AUR Notes

- Do not prompt for configuration inside `pacman` install hooks. That breaks unattended package installs.
- ChangeWall handles this correctly by asking for the wallpaper folder on first launch inside the user session.
- The package can still install autostart and shortcut-ready defaults so the user only needs to pick the folder once.

## Notes

- The current implementation applies the same wallpaper to all Plasma desktops.
- Multi-monitor wallpaper differentiation is not implemented yet; the transition overlay already spans every screen so the UX is ready for that extension.
