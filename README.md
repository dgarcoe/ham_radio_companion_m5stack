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
- **Bearing & distance** — given your QTH grid (set once in Settings), enter
  any 4- or 6-character Maidenhead grid and get the great-circle bearing,
  16-point compass label, distance (km + mi), and the long-path heading. All
  offline — pure spherical-trig math.
- **NOAA space-weather alerts** — fetches
  `services.swpc.noaa.gov/products/alerts.json` every 15 minutes and lists
  recent alerts/warnings/watches/summaries with severity-coded codes (red for
  warnings, orange for alerts/watches).
- **Contest calendar** — offline, computed locally from a curated list of
  ~19 major HF contests with per-contest recurrence rules. Lists what's
  running now and what's coming up in the next ~6 months, with mode badges
  and "starts in N days" / "RUNNING - Nh left" relative times.
- **Grayline map** — equirectangular world map with the day/night
  terminator drawn from the current sun position (declination + sub-solar
  longitude with a small equation-of-time correction). Sun, your QTH, and
  the 18 NCDXF beacons are plotted as dots so you can see at a glance
  what's currently in dawn/dusk grayline.
- **Battery indicator** — vertical bar in the launcher banner top-right,
  green/orange/red by level, with a "+" suffix on the readout while charging.
- **Alerts** — tone + on-screen banner whenever a *DX cluster* spot matches
  one of your user-defined rules (band / mode / DXCC prefix / callsign
  substring).
- **Settings** — on-device editor for WiFi creds, callsign, QTH grid,
  cluster, UTC offset, sound, the propagation URL, and a **Home Tiles**
  editor that hides/shows launcher tiles (Settings is always visible).
  All edits use a touch keyboard or single-tap toggles.

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
  "myGrid": "",
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

### Bearing & Distance
Two cards at the top — `FROM (your QTH)` (read from `myGrid` in config; tap
to set it inline) and `TO`, where you enter any Maidenhead grid (4 or 6
chars). The result card shows the great-circle bearing in degrees with a
16-point compass label, the short-path distance in km and miles, and the
long-path heading + distance. Pure offline math, no network needed.

### Space Weather (NOAA SWPC)
Fetches `services.swpc.noaa.gov/products/alerts.json` every 15 minutes. Each
entry shows the SWPC product code (color-coded: red for warnings, orange for
alerts and watches, dim grey for summaries), the issue timestamp, and a
one-line summary pulled from the message body. Tap **Refresh** for an
immediate refresh, scroll with `^` / `v`.

### Contest Calendar
A scrolling list of major HF contests. Computed locally from a curated set
of recurrence rules ("first Saturday of November, 30 hours") - no network
required, the only dependency is NTP for the system clock. Each row shows
the contest name, mode badge (CW cyan / SSB green / RTTY orange / MIX dim),
the start day-of-week + UTC start time, and a "starts in N days" or
"RUNNING - Nh left" indicator that ticks once a minute. Coverage includes
ARRL DX, CQ WW, CQ WPX, IARU HF, Sweepstakes, Field Day, WAE, ARRL 10m /
160m, RTTY Roundup, and a few NA QSO Parties.

### Grayline
A 320 x 218 equirectangular world map with day/night shading and the
current terminator. Computed from the sun's declination (Cooper's formula)
and sub-solar longitude (clock + a small equation-of-time correction) -
accurate to better than half a degree. The orange disc is the sub-solar
point; if `myGrid` is set, your QTH is a green dot. The 18 NCDXF beacon
stations are plotted as small white pixels so you can spot which beacons
are in grayline at any moment.

### Satellites
Predicts upcoming passes for a curated list of popular amateur satellites
over your QTH (read from `myGrid`). On the first WiFi-connected boot, TLEs
are pulled from Celestrak (`amateur` group, fall-back to `stations`) and
cached on LittleFS; subsequent boots use the cache and refresh at most
once a day. Each row shows the satellite name, AOS day + UTC time, "in
Nm/h" relative start, max elevation (color-coded: green >=30 deg, orange
>=15 deg), AOS->LOS compass headings, and pass duration. The propagator
models the dominant J2 secular perturbations and the linear drag term
from the TLE - accurate to a couple of minutes for a 1-2 day horizon.
The pass list is recomputed every five minutes.

### Home Tiles
A "Home Tiles" entry in Settings opens a list of every launcher tile with
an `ON` / `OFF` pill. Tap to hide a tile from the launcher (the grid
reflows automatically). The Settings tile itself is locked `ON` so you
can always get back to the editor.

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
WiFi, callsign, cluster host/port, UTC offset, sound on/off, propagation
URL, and **Home Tiles**. Each row opens an editor (full keyboard for
strings, +/- buttons for ints, a single tap toggles **Sound**, the Home
Tiles row jumps to the tile-visibility editor). Two scroll buttons in the
header walk through the list.

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
  noaa.h/.cpp           NOAA SWPC alerts fetcher (JSON via HTTPS)
  contests.h/.cpp       Offline major-HF-contest calendar with recurrence rules
  sun.h/.cpp            Solar declination, sub-solar longitude, altitude helpers
  geo.h/.cpp            Maidenhead grid <-> lat/lon, great-circle bearing/distance
  sat_sgp4.h/.cpp       Compact SGP4-style propagator (J2 + drag, ECI/ECF/LLA)
  satellites.h/.cpp     Celestrak TLE fetcher + pass-window computation
  alerts.h/.cpp         filter engine for DX spots, beep + banner queue
  ui/
    ui.h                public UI API (Screen enum, setScreen/goHome, modal editors)
    ui_internal.h       layout constants, colors, screen handler decls
    ui.cpp              chrome (back-button + title + mini-clock), redraw scheduler,
                        alert banner, hardware-button routing
    icons.h/.cpp        16x16 monochrome icons (house/DX/sun/bell/gear/refresh/
                        beacon-tower/pine-tree/compass-rose/warning-triangle/
                        trophy/globe-with-terminator)
    keyboard.cpp        modal on-screen keyboard + numeric editor
    screen_launcher.cpp home: big clock + callsign + tile grid
    screen_dx.cpp       DX spot list with band/mode filter pills
    screen_prop.cpp     solar + HF band conditions
    screen_beacons.cpp  NCDXF "now transmitting" panel + station roster
    screen_pota.cpp     POTA activator list
    screen_bearing.cpp  Maidenhead bearing/distance/long-path calculator
    screen_noaa.cpp     NOAA SWPC space-weather alerts
    screen_contests.cpp Upcoming-and-running HF contest list
    screen_grayline.cpp Day/night world map with terminator + dots
    screen_satellites.cpp Upcoming-pass list (AOS/LOS/max elev/duration)
    screen_alerts.cpp   rules list + history (with Rules/History toggle)
    screen_settings.cpp scrollable settings list + edit dispatch
    screen_home_tiles.cpp ON/OFF toggles for each launcher tile
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
