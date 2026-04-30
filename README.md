# Ham Radio Companion for M5Stack Core2

A standalone ham-radio dashboard for the
[M5Stack Core2](https://docs.m5stack.com/en/core/core2) (ESP32, 320x240 capacitive
touchscreen, speaker).  Inspired by the various "ham radio clock" projects, it
combines four useful tools in one device:

- **UTC clock** with date and your callsign on the home screen.
- **DX cluster monitor** — connects to any standard telnet DX cluster and shows
  the most recent spots (band, frequency, callsign, mode, age) with on-screen
  band/mode filters that persist across reboots.
- **HF propagation dashboard** — fetches the
  [HamQSL solar XML feed](https://www.hamqsl.com/solar.html) and shows SFI,
  sunspots, A / K indices, X-Ray, S/N, MUF, aurora, plus the day/night
  conditions table for 80-40 / 30-20 / 17-15 / 12-10 m.
- **Alerts** — tone + on-screen banner whenever a spot matches one of your
  user-defined rules (band / mode / DXCC prefix / callsign substring).

Configuration can be supplied either by editing `data/config.json` and uploading
it to the device's LittleFS partition, or live on-device via a touch keyboard.
Either way the settings persist across reboots — no recompile needed once the
firmware is flashed. Each tab has a small icon above its label so the device
is usable at a glance.

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

# Open the serial monitor (optional)
pio device monitor
```

The PlatformIO config (`platformio.ini`) targets `m5stack-core2` with the
Arduino framework and pulls in `M5Unified` and `ArduinoJson`.

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
  "propagationUrl": "http://www.hamqsl.com/solarxml.php",
  "utcOffset": 0,
  "soundEnabled": true,
  "alerts": [
    { "name": "20m CW",     "band": "20m", "mode": "CW",  "prefix": "",   "callMatch": "", "enabled": false },
    { "name": "VK on 15m",  "band": "15m", "mode": "any", "prefix": "VK", "callMatch": "", "enabled": false },
    { "name": "FT8 anyway", "band": "any", "mode": "FT8", "prefix": "",   "callMatch": "", "enabled": false }
  ]
}
```

Any field you don't set keeps its built-in default. The boot-time serial log
will print `Config : loaded from file` (or `nvs` / `defaults`) so you can
verify which source is active.

The load order is: **`/config.json` on LittleFS → NVS → built-in defaults**.
Whenever you change a setting on-device, the new value is written to *both*
NVS and the JSON file, so the file always reflects current state and a
`pio run -t buildfs` afterwards would let you fetch it back.

### Option B — On-device editor

1. Power the device on.
2. Tap the **Set** tab (gear icon).
3. Tap **WiFi SSID** and enter the SSID using the on-screen keyboard, then tap
   **OK**.
4. Tap **WiFi Pass** and enter the WiFi password.
5. Tap **My Callsign** — used to log into the DX cluster and shown on Home.
6. (Optional) Adjust **Cluster Host** / **Cluster Port** if you don't want the
   default `dxc.k0xm.net:7300`.  Most public clusters work with this client
   (W3LPL, NC7J, K0XM, etc.).
7. (Optional) Adjust **Prop URL** if HamQSL is unreachable from your network
   — any source returning the HamQSL XML schema works.
8. Tap the **Home** tab (house icon). Within ~10 s the WiFi line should show your IP and
   RSSI; the DXC line should show *connected*; and the Sun line should show
   `SFI / A / K`.

## Tabs in detail

### Home
Big UTC clock (NTP-synced), date, your callsign, and a status block with WiFi,
cluster, and current solar numbers.

### DX
Live list of the most recent ~200 spots received from the cluster. Two filter
rows at the top let you pick a band (`any` / `160m` / ... / `2m`) and a mode
(`any` / `CW` / `SSB` / `FT8` / `FT4` / `RTTY` / `DIGI` — the last bucket
matches all common digital modes). Tap the value pill to step forward, or the
`<` / `>` buttons for previous/next. The right of the band row shows
*matching / total* spots. Use the `^` / `v` buttons on the right to scroll the
list.

Columns: band, frequency (kHz), DX callsign, mode (decoded from the comment or
guessed from the frequency), and age. Filter selections persist in NVS and the
config file.

### Prop
Solar conditions and HF band conditions table. Tap **Refresh** to force a new
fetch (it auto-refreshes every 30 minutes). Cells use color: green = Good,
orange = Fair, red = Poor.

### Alerts
- **Rules** view: lists each rule with its filters. Tap the `ON`/`OFF` pill on
  the left to toggle a rule. Adding new rules from the device UI is on the
  to-do list — for now the easiest way is to edit them via the JSON stored in
  NVS (see *Adding alerts* below).
- **History** view: most recent matches, newest first.
- When a spot matches an enabled rule, the device beeps (twice) and shows an
  orange banner at the bottom of the screen for ~3.5 seconds.

### Settings
WiFi, callsign, cluster host/port, UTC offset, sound on/off, and propagation
URL. Each row opens an editor (full keyboard for strings, +/- buttons for
ints, a single tap toggles **Sound**).

## Adding alerts

Out of the box (built-in defaults) there is one disabled example rule named
*Example: 20m CW*; if you've uploaded the shipped `data/config.json` you'll
get three example rules. Toggle them on from the Alerts tab to see how alerts
look while you're connected to a cluster.

A rule has these fields (all optional except `name`):

| field      | meaning                                                |
|------------|--------------------------------------------------------|
| `name`     | display name shown in the rules list and banner        |
| `band`     | exact band tag, e.g. `20m`, `40m`, `6m`, or `any`      |
| `mode`     | `CW` / `SSB` / `FT8` / `RTTY` / ... or `any`           |
| `prefix`   | DXCC prefix to match against the start of the callsign |
| `callMatch`| substring to match anywhere inside the callsign        |
| `enabled`  | `true` / `false`                                       |

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
  propagation.h/.cpp    HamQSL XML fetcher + parser
  alerts.h/.cpp         filter engine for spots, beep + banner queue
  ui/
    ui.h                public UI API
    ui_internal.h       layout constants, colors, screen handler decls
    ui.cpp              tab bar, redraw scheduler, banner overlay
    icons.h/.cpp        16x16 monochrome icons (home, DX, sun, bell, gear, refresh)
    keyboard.cpp        modal on-screen keyboard + numeric editor
    screen_home.cpp     UTC clock + status block
    screen_dx.cpp       scrollable spot list
    screen_prop.cpp     solar + band conditions
    screen_alerts.cpp   rules list + history
    screen_settings.cpp settings rows + edit dispatch
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
  `WiFiClientSecure::setInsecure()` because we're only reading public solar
  data — if you need certificate validation, pass a CA bundle in
  `propagation.cpp`.

## License

MIT.
