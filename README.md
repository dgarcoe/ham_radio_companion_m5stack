# Ham Radio Companion for M5Stack Core2

A standalone ham-radio dashboard for the
[M5Stack Core2](https://docs.m5stack.com/en/core/core2) (ESP32, 320x240 capacitive
touchscreen, speaker).  Inspired by the various "ham radio clock" projects, it
combines four useful tools in one device:

- **UTC clock** with date and your callsign on the home screen.
- **DX cluster monitor** — connects to any standard telnet DX cluster and shows
  the most recent spots (band, frequency, callsign, mode, age).
- **HF propagation dashboard** — fetches the
  [HamQSL solar XML feed](https://www.hamqsl.com/solar.html) and shows SFI,
  sunspots, A / K indices, X-Ray, S/N, MUF, aurora, plus the day/night
  conditions table for 80-40 / 30-20 / 17-15 / 12-10 m.
- **Alerts** — tone + on-screen banner whenever a spot matches one of your
  user-defined rules (band / mode / DXCC prefix / callsign substring).

All settings are configured on-device via a touch keyboard and persisted in
NVS — no recompile needed once the firmware is flashed.

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

## First-time setup

1. Power the device on. The first time, all fields are blank/default and there
   is one disabled example alert rule.
2. Tap the **Set** tab.
3. Tap **WiFi SSID** and enter the SSID using the on-screen keyboard, then tap
   **OK**.
4. Tap **WiFi Pass** and enter the WiFi password.
5. Tap **My Callsign** and enter your callsign — it will be used to log into
   the DX cluster and is displayed on the Home screen.
6. (Optional) Adjust **Cluster Host** / **Cluster Port** if you don't want the
   default `dxc.k0xm.net:7300`.  Most public clusters work with this client
   (W3LPL, NC7J, K0XM, etc.).
7. (Optional) Adjust the **Prop URL** if HamQSL is unreachable from your
   network — any source returning the HamQSL XML schema works.
8. Tap the **Home** tab. Within ~10 s the WiFi line should show your IP and
   RSSI; the DXC line should show *connected*; and the Sun line should show
   `SFI / A / K`.

## Tabs in detail

### Home
Big UTC clock (NTP-synced), date, your callsign, and a status block with WiFi,
cluster, and current solar numbers.

### DX
Live list of the most recent ~200 spots received from the cluster. Use the `^`
/ `v` buttons on the right to scroll. Columns: band, frequency (kHz), DX
callsign, mode (decoded from the comment or guessed from the frequency), and
age.

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

Out of the box there is one disabled example rule named *Example: 20m CW*.
Toggle it on from the Alerts tab to see how alerts look while you're connected
to a cluster.

A rule has these fields (all optional except `name`):

| field      | meaning                                                |
|------------|--------------------------------------------------------|
| `name`     | display name shown in the rules list and banner        |
| `band`     | exact band tag, e.g. `20m`, `40m`, `6m`, or `any`      |
| `mode`     | `CW` / `SSB` / `FT8` / `RTTY` / ... or `any`           |
| `prefix`   | DXCC prefix to match against the start of the callsign |
| `callMatch`| substring to match anywhere inside the callsign        |
| `enabled`  | `true` / `false`                                       |

Until in-device rule editing lands, you can preload rules at compile time by
editing the seeded list in `src/config.cpp` (the `if (json.isEmpty())` block)
and reflashing.

## Project layout

```
platformio.ini          PlatformIO target / lib deps
src/
  main.cpp              setup/loop, wires everything together
  config.h/.cpp         persisted AppConfig (Preferences/NVS, JSON-encoded)
  wifi_manager.h/.cpp   WiFi STA + NTP time sync
  dx_cluster.h/.cpp     telnet client + DX spot line parser + band/mode helpers
  propagation.h/.cpp    HamQSL XML fetcher + parser
  alerts.h/.cpp         filter engine for spots, beep + banner queue
  ui/
    ui.h                public UI API
    ui_internal.h       layout constants, colors, screen handler decls
    ui.cpp              tab bar, redraw scheduler, banner overlay
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
- The propagation feed is fetched over HTTP (the default URL). If you switch
  to HTTPS, ensure the HTTPClient library is built with the proper certs.

## License

MIT.
