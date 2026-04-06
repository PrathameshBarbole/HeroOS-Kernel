# HeroOS App Naming Guide

## Naming Philosophy

HeroOS app names follow a simple, intentional principle:

> **User-facing apps get premium, standalone names.**
> **OS-level tools keep the `Hero` prefix.**

This gives the system a mature, polished feel — like macOS where apps are
called "Files", "Photos", "Terminal" rather than "AppleFiles" or "macFiles".
The `Hero` prefix is a mark of ownership for core platform tools, not a
generic prefix applied to everything.

---

## App Name Registry

### User Apps (standalone names)

These apps are presented to users. Their names are short, descriptive, and
premium — they stand on their own without needing the OS name in them.

| Binary    | Display Name | One-line description |
|-----------|-------------|----------------------|
| `files`   | **Files**   | File manager — browse, search, manage your filesystem |
| `terminal`| **Terminal**| Terminal emulator with tabs, splits, and HeroShell inside |
| `quill`   | **Quill**   | Code and text editor with LSP and syntax highlighting |
| `lens`    | **Lens**    | Privacy-first web browser with built-in ad blocking |
| `prism`   | **Prism**   | System settings — one unified place for all config |
| `pulse`   | **Pulse**   | System monitor: CPU, RAM, processes, network at a glance |
| `wave`    | **Wave**    | Music and video player with playlists and visualiser |

### HeroOS Platform Tools (`Hero` prefix — intentional)

These are OS-level tools that are core to the HeroOS identity. They are not
general-purpose apps — they are HeroOS-specific infrastructure.

| Binary      | Name          | Why it has the Hero prefix |
|-------------|---------------|---------------------------|
| `heroshell` | **HeroShell** | The HeroOS system shell — not just any shell |
| `heropkg`   | **HeroPkg**   | The HeroOS package manager — tied to Hero repos |
| `heroserve` | **HeroServe** | The HeroOS zero-config server — a Hero brand feature |

Command shorthand: all three are accessible via the `hero` meta-command:
```
hero serve       → HeroServe
hero pkg install → HeroPkg
hero shell       → HeroShell
```

### UI System Components (internal names, not user-app names)

These run as system services. Users interact with their effects (windows,
notifications, launcher) but don't "open" them by name.

| Internal ID | Name       | Role |
|-------------|-----------|------|
| `canvas`    | **Canvas** | Wayland-inspired compositor — draws everything |
| `aura`      | **Aura**   | Desktop environment — windows, workspaces, panels |
| `orbit`     | **Orbit**  | App launcher — Super+Space, fuzzy search |
| `echo`      | **Echo**   | Notification system — slide-in cards, history |

---

## Naming Rules

1. **User apps**: short English word that evokes the app's purpose.
   - ✅ `Files`, `Terminal`, `Quill`, `Lens`, `Prism`, `Pulse`, `Wave`
   - ❌ `HeroFiles`, `HeroTerminal`, `HeroEditor`

2. **HeroOS platform tools**: prefix with `Hero` + descriptive noun.
   - ✅ `HeroShell`, `HeroPkg`, `HeroServe`
   - These are unique to HeroOS; the prefix is part of the brand.

3. **System services**: single evocative word, lowercase as binary name.
   - ✅ `canvas`, `aura`, `orbit`, `echo`
   - These never appear in menus; only their effects are visible.

4. **The OS itself** is always **HeroOS** (one word, capital H and OS).
   - ✅ `HeroOS v0.1.0`
   - ❌ `Hero OS`, `heroOS`, `Heroos`

5. **Future apps** should follow the standalone-name pattern:
   - Calendar app → `Chronicle` or `Cal`
   - Photo viewer → `Gallery` or `Snap`
   - Notes app → `Ink` or `Notes`
   - Maps → `Atlas`
   - Camera → `Capture`
   - Email → `Inbox`

---

## App Icon Paths

App icons are stored at `/usr/share/icons/<app-id>.png`.

| App     | Icon path |
|---------|-----------|
| Files   | `/usr/share/icons/files.png` |
| Terminal| `/usr/share/icons/terminal.png` |
| Quill   | `/usr/share/icons/quill.png` |
| Lens    | `/usr/share/icons/lens.png` |
| Prism   | `/usr/share/icons/prism.png` |
| Pulse   | `/usr/share/icons/pulse.png` |
| Wave    | `/usr/share/icons/wave.png` |

---

## Desktop Entry Format (future `.desktop` equivalent)

HeroOS uses a lightweight `.app` descriptor format:

```toml
[App]
id          = "quill"
name        = "Quill"
description = "Code and text editor"
icon        = "/usr/share/icons/quill.png"
exec        = "/bin/quill"
categories  = ["dev", "utility"]
version     = "0.1.0"
```

Files are stored in `/usr/share/apps/<id>.app`.
