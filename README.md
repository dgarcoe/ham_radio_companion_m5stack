# Ham Radio Companion for M5Stack Core2

A standalone ham-radio dashboard for the
[M5Stack Core2](https://docs.m5stack.com/en/core/core2) (ESP32, 320x240 capacitive
touchscreen, speaker). Inspired by the various "ham radio clock" projects, it
combines several useful tools in one device:

- **Home launcher** — big UTC clock, your callsign, the date, and a 3×2 grid
  of tappable tiles for the rest of the tools. Each tile shows a small
  live-status badge so you don't have to drill in to see if anything's going
  on.
- **DX cluster monitor** — connects to any standard telnet DX cluster and
  shows the most recent spots (band, frequency, callsign, mode, age) with
  band/mode filters that persist across reboots.
- **HF propagation dashboard** — fetches the
  [HamQSL solar XML feed](https://www.hamqsl.com/solar.html) and shows SFI,
  sunspots, A / K indices, X-Ray, S/N, MUF, aurora, plus the day/night
  conditions table for 80-40 / 30-20 / 17-15 / 12-10 m.
- **NCDXF / IARU beacon monitor** — shows which of the 18 international
  beacons is transmitting *right now* on each of the 5 HF beacon bands
  (14.100, 18.110, 21.150, 24.930, 28.200 MHz), with a 10-second slot
  countdown bar. The schedule is purely time-based, so it works without any
  internet beyond the initial NTP sync.
- **POTA spot feed** — periodically pulls the live activator-spot list from
  `https://api.pota.app/spot/activator` and shows park reference, activator,
  frequency, mode, and location.
- **Alerts** — tone + on-screen banner whenever a *DX cluster* spot matches
  one of your user-defined rules (band / mode / DXCC prefix / callsign
  substring).
- **Settings** — on-device editor for WiFi creds, callsign, cluster, UTC
  offset, sound, and the propagation URL, with a touch keyboard.

Configuration can be supplied either by editing `data/config.json` and
uploading it to the device's LittleFS partition, or live on-device via the
touch keyboard. Either way the settings persist across reboots — no recompile
needed once the firmware is flashed.

## Hardware

- **M5Stack Core2** (the original Core2 or Core2 v1.1).
- WiFi access to the Internet.

## Build & flash

The project uses [PlatformIO](https://platformio.org/).

```bash
# Install PlatformIO if you don't have it
pip install -U platformio

# Build & flash (USB-C)
pio run -t upload

# Optionally upload the default config too
pio run -t uploadfs

# Open the serial monitor
pio device monitor
```

The PlatformIO config (`platformio.ini`) targets `m5stack-core2` with the
Arduino framework and pulls in `M5Unified` and `ArduinoJson`.

## Navigation

The home screen is a launcher: the top banner has the big clock + callsign +
date, and below it is a grid of tiles, one per tool. Tap a tile to enter that
tool. Inside a tool, the top-left **&lt; Home** button (or the hardware
**Button A** under the screen) takes you back to the launcher. The top-right
of every non-launcher screen shows a compact `HH:MMZ` mini-clock so you
always have UTC visible.

## Configuration

You can configure the device two ways. Both update the same fields and either
can be used in isolation.

### Option A — `data/config.json` (recommended for first-time setup)

Edit `data/config.json` in the repo, then upload it to the device's LittleFS
partition:

```bash
pio run -t uploadfs
```

The file's structure (defaults shown):

```json
{
  "wifiSsid": "",
  "wifiPass": "",
  "myCallsign": "N0CALL",
  "clusterHost": "dxc.k0xm.net",
  "clusterPort": 7300,
  "propagationUrl": "https://www.hamqsl.com/solarxml.php",
  "utcOffset": 0,
  "soundEnabled": true,
  "filterBand": "any",
  "filterMode": "any",
  "alerts": [
    { "name": "20m CW",     "band": "20m", "mode": "CW",  "prefix": "",   "callMatch": "", "enabled": false },
    { "name": "VK on 15m",  "band": "15m", "mode": "any", "prefix": "VK", "callMatch": "", "enabled": false },
    { "name": "FT8 anyway", "band": "any", "mode": "FT8", "prefix": "",   "callMatch": "", "enabled": false }
  ]
}
```

Any field you don't set keeps its built-in default. The boot-time serial log
prints `Config : loaded from file` (or `nvs` / `defaults`) so you can verify
which source is active.

The load order is: **`/config.json` on LittleFS → NVS → built-in defaults**.
Whenever you change a setting on-device, the new value is written to *both*
NVS and the JSON file.

### Option B — On-device editor

1. Power the device on. You'll land on the launcher.
2. Tap the **Settings** tile (gear icon).
3. Tap **WiFi SSID** → enter SSID with the on-screen keyboard → **OK**.
4. Tap **WiFi Pass** → enter the password.
5. Tap **My Callsign**.
6. (Optional) Adjust **Cluster Host / Port** if you don't want the default
   `dxc.k0xm.net:7300`. Most public clusters work (W3LPL, NC7J, K0XM, ...).
7. (Optional) Adjust **Prop URL** if HamQSL isn't reachable.
8. Tap **&lt; Home**. Within ~10 s the WiFi/DX/Sun status should turn green on
   each tile.

## Tools in detail

### DX Cluster
Live list of the most recent ~200 spots from the cluster. Two filter pills at
the top let you pick a band (`any` / `160m` / ... / `2m`) and a mode (`any` /
`CW` / `SSB` / `FT8` / `FT4` / `RTTY` / `DIGI` — that bucket matches all
common digital modes). Tap the value pill to step forward; `<` / `>` step
backward/forward. The right of the band row shows *matching / total* counts.
`^` / `v` on the right of the list scroll. Filter selections persist in NVS
and the config file.

### Propagation
Solar conditions and HF band conditions. Tap **Refresh** to force a new fetch
(it auto-refreshes every 30 minutes). Day/night condition cells use color:
green = Good, orange = Fair, red = Poor.

### NCDXF Beacons
Shows the 18 international beacons (4U1UN, VE8AT, W6WX, KH6RS, ZL6B, ...). The
top panel highlights *who is transmitting on each of the 5 beacon bands right
now*; the bar at the bottom of that panel counts down the current 10-second
slot. Below is the full station roster — the active stations are colored cyan
and tagged with the band they're currently on.

The schedule is computed locally from UTC; once NTP has synced (a few seconds
after WiFi connects) the display is accurate to within a second.

### POTA
Live Parks On The Air activator list, refreshed every minute from
`api.pota.app`. Each row shows the park reference (e.g. `K-1234`), the
activator's callsign, frequency, mode, the park name, and country code. Tap
**Refresh** to force an immediate refetch.

### Alerts
- **Rules** view: lists each rule with its filters. Tap the `ON`/`OFF` pill
  on the left to toggle a rule. New rules are added by editing
  `data/config.json` and re-uploading the FS.
- **History** view: the most recent matches, newest first.
- When a spot matches an enabled rule, the device beeps (twice) and shows an
  orange banner at the bottom of the screen for ~3.5 seconds.

Alerts presently match only the DX cluster feed; routing POTA spots through
the alert engine is a logical next step.

### Settings
WiFi, callsign, cluster host/port, UTC offset, sound on/off, and propagation
URL. Each row opens an editor (full keyboard for strings, +/- buttons for
ints, a single tap toggles **Sound**). Two scroll buttons in the header walk
through the list.

## Adding alerts

A rule has these fields (all optional except `name`):

| field       | meaning                                                  |
|-------------|----------------------------------------------------------|
| `name`      | display name shown in the rules list and banner          |
| `band`      | exact band tag, e.g. `20m`, `40m`, `6m`, or `any`        |
| `mode`      | `CW` / `SSB` / `FT8` / `RTTY` / ... or `any`             |
| `prefix`    | DXCC prefix to match against the start of the callsign   |
| `callMatch` | substring to match anywhere inside the callsign          |
| `enabled`   | `true` / `false`                                         |

Until in-device rule editing lands, the easiest way to add or change rules is
to edit `data/config.json` and run `pio run -t uploadfs`.

## Project layout

```
platformio.ini          PlatformIO target / lib deps
data/
  config.json           default user config; uploaded with `pio run -t uploadfs`
src/
  main.cpp              setup/loop, wires everything together
  config.h/.cpp         persisted AppConfig (LittleFS file + NVS, JSON-encoded)
  wifi_manager.h/.cpp   WiFi STA + NTP time sync
  dx_cluster.h/.cpp     telnet client + DX spot line parser + band/mode helpers
  propagation.h/.cpp    HamQSL XML fetcher + parser (handles HTTP->HTTPS redirects)
  beacons.h/.cpp        NCDXF beacon table + slot/station math
  pota.h/.cpp           POTA activator-spot fetcher (JSON via HTTPS)
  alerts.h/.cpp         filter engine for DX spots, beep + banner queue
  ui/
    ui.h                public UI API (Screen enum, setScreen/goHome, modal editors)
    ui_internal.h       layout constants, colors, screen handler decls
    ui.cpp              chrome (back-button + title + mini-clock), redraw scheduler,
                        alert banner, hardware-button routing
    icons.h/.cpp        16x16 monochrome icons (house/DX/sun/bell/gear/refresh/
                        beacon-tower/pine-tree)
    keyboard.cpp        modal on-screen keyboard + numeric editor
    screen_launcher.cpp home: big clock + callsign + tile grid
    screen_dx.cpp       DX spot list with band/mode filter pills
    screen_prop.cpp     solar + HF band conditions
    screen_beacons.cpp  NCDXF "now transmitting" panel + station roster
    screen_pota.cpp     POTA activator list
    screen_alerts.cpp   rules list + history (with Rules/History toggle)
    screen_settings.cpp scrollable settings list + edit dispatch
```

## Notes & caveats

- The cluster login is intentionally simple: when the cluster prompts for a
  call (`login:` / `your call`), we send the configured callsign followed by
  `\r\n`. Most clusters accept that — if yours requires more (e.g. a password
  for a private cluster), you'll need to extend `dx_cluster.cpp`.
- Mode detection is best-effort: if the spot's comment carries a mode keyword
  it is used verbatim; otherwise we guess from sub-band conventions.
- HamQSL's XML returns the `<electonflux>` tag with that spelling; we accept
  both spellings to be safe if they ever fix it.
- The propagation feed is fetched over HTTPS by default
  (`https://www.hamqsl.com/solarxml.php`). The HTTP redirect chain
  (e.g. 301 → HTTPS) is handled manually so plain-HTTP URLs work too. We use
  `WiFiClientSecure::setInsecure()` because we're only reading public data —
  if you need certificate validation, pass a CA bundle in `propagation.cpp`
  / `pota.cpp`.
- The NCDXF beacon code assumes the published 3-minute / 10-second-slot
  schedule is in effect (it has been since 1995). The data table includes
  call, country, grid, and rough lat/lon for each station so future map
  features can plot them.

## License

MIT.
