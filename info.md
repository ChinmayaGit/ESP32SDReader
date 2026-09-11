# ESP32 SD Reader — how it was built

Engineering notes: architecture, how a request is served, problems we actually hit, and what this hardware cannot do. For day-to-day use see [README.md](README.md).

## Goal

A pocket web NAS: ESP32 + SPI microSD, phone connects over Wi-Fi, no PC after flash. The UI had to work even when the card failed to mount, so pins and Wi-Fi could be fixed from the browser.

It is **not** USB mass storage and **not** a real NAS. Throughput is Wi-Fi + SPI jumper wires.

## Hardware that worked

| Piece | What we used |
|--------|----------------|
| MCU | ESP32-D0WDQ6, 240 MHz, Arduino core 3.3.11 |
| Serial | `/dev/cu.usbserial-0001`, MAC `58:bf:25:9f:64:b4` |
| Card | ~128 GB FAT32 (`SD01288`) |
| Module | Cheap 6-pin SPI adapter with AMS1117 |
| Power | **VIN / 5V**, not 3V3 (3V3 brownouts / flaky mounts) |
| SPI | CS 5, SCK 18, MISO 19, MOSI 23 |
| Clock | Probe 40 → 26 → 20 → 16 MHz. **16 MHz** is the highest that stayed up on jumper wires |

Firmware tries HSPI then VSPI, and swapped MOSI/MISO, at 400 kHz first, then raises the clock.

## Software layout

| File | Role |
|------|------|
| `ESP32SDReader.ino` | Wi-Fi, SD mount, HTTP API, FTP start, Range streaming |
| `webui.h` | Single-page UI compiled into flash (`INDEX_HTML`) |
| `qrcode.h` | QR helper used by `/api/qr` |
| SimpleFTPServer 3.0.2 | FTP (must be compiled as **STORAGE_SD**) |

Preferences NVS keys store hotspot SSID/password, Wi-Fi mode, home SSID/password, and SPI pins.

## How it works

### Boot

1. Load settings from NVS.
2. Mount SD over SPI (`mountSdSpi()`).
3. Start Wi-Fi (`startWifi()`): AP, STA, or AP+STA. If STA is requested and the SSID is missing, fall back to hotspot.
4. Start mDNS hostname `esp32-sd`.
5. Start FTP only if the card mounted (`startFtp()` ends the old sockets first, then `begin`).
6. Bind HTTP on port 80.

### HTTP

Arduino `WebServer` on port 80. The UI is `send_P` of `INDEX_HTML`. JSON APIs:

| Path | Purpose |
|------|---------|
| `/api/status` | Card size, IPs, FTP hints |
| `/api/list?path=` | Directory listing |
| `/api/file?path=` | Download / stream (Range) |
| `/media/<path>` | Same file, **real filename in the URL** (what VLC needs) |
| `/v.mp4?path=` (and other `/v.*`) | Older VLC-friendly suffix; Play now prefers `/media/` |
| `/api/upload` | Multipart upload |
| `/api/delete`, `/api/mkdir` | Mutate the card |
| `/api/settings`, `/api/scan`, `/api/wifi/join` | Wi-Fi and pins |
| `/api/qr?kind=` | Wi-Fi / web / FTP QR |
| Captive probes | `/generate_204`, `/hotspot-detect.html`, … return 204/OK so phones do not swallow the UI |

`serveSdFile()` implements `Range: bytes=` and answers `206 Partial Content`. That is why VLC can play HEVC over HTTP: it seeks to the `moov` atom.

Play in the UI opens:

```
http://<the-ip-you-used>/media/<urlencoded-path>/<filename>
```

Transfers use a 16 KB buffer. Serial logging is kept out of the send loop.

Path names with `%` encodings (for example an en-dash `%E2%80%93`) go through `urlDecode` / `resolveSdPath` so list, play, and delete still find the file.

### Wi-Fi modes

- **ap** — hotspot `192.168.4.1` only
- **both** — hotspot + join home Wi-Fi (two IPs)
- **sta** — home Wi-Fi only; if that network is gone, hotspot comes back

AP and STA share one radio channel. `http://192.168.4.1` exists only on the hotspot interface. From home Wi-Fi you must use the STA IPv4 (example `192.168.1.29`).

After a Wi-Fi change the firmware restarts HTTP (`server.close()` / `begin()`) and FTP (`ftpSrv.end()` / `begin()`). If you only called `begin()` again, ESP32 `WiFiServer` would keep `_listening` on a dead socket.

PASV/EPSV pick the data-channel IP from the FTP client’s subnet (AP `192.168.4.x` vs LAN), not always the hotspot IP.

### FTP

SimpleFTPServer listens on port 21. Data port is passive **50009**.

Credentials: user `sd`, password `sdreader1`.

The sketch `#define DEFAULT_STORAGE_TYPE_ESP32 5` does **not** apply to the library `.cpp`. Arduino compiles the library as its own translation unit. The library default is **FFat**. FTP would log in and list an empty flash volume while the web UI showed the SD card.

Fix: in `Arduino/libraries/SimpleFTPServer/FtpServerKey.h` set

```c
#define DEFAULT_STORAGE_TYPE_ESP32  STORAGE_SD
```

Reinstalling the library wipes that. Check it after every library update.

Also patched in that copy:

- EPSV (Finder / some phones)
- PASV IP from client subnet
- `openDir` actually checks `isDirectory()`
- REST advertised in FEAT; RETR seeks to `restartPos`
- `doRetrieve()` write retries (do not lose this on reinstall)

Use FileZilla or Cyberduck, passive mode, plain FTP. Finder and iOS Files are unreliable.

## Features (product)

- Phone-sized file browser with list/grid and sort
- Upload / download / delete / mkdir
- Watch-mode vertical swipe of videos in a folder (one `<video src>` at a time — ESP32 connection limit)
- Play → HTTP `/media/…` for VLC
- Connect sheet: FTP, VLC HTTP, copy, QR
- Hotspot fallback when home SSID is out of range
- mDNS `esp32-sd.local`
- Captive-portal probes do not dump the whole UI

## Issues we hit

### SD would not mount

- Module on 3V3 instead of VIN
- Loose jumpers after rewiring
- CS left on 3.3V from an SDMMC experiment instead of GPIO 5
- SPI clock too high (20 MHz+ failed; 16 MHz OK)

### SDMMC 1-bit experiment (failed)

Tried CLK 14, CMD 15, D0 2, CS pulled high. `sdmmc_init_ocr` returned **0x107 timeout** even at 400 kHz. This adapter is SPI-buffered; it is not a native MMC slot. Firmware is SPI-only again. A real 4-bit slot would need D0–D3 and care with GPIO 12 (strap pin).

Boot used to wait ~10–15 s retrying MMC. That probe was removed.

### Joined Wi-Fi, HTTP “dead”

People stayed on Airtel and opened `http://192.168.4.1`. That address is only on the hotspot.

mDNS in AP+STA often advertises the AP IP, so `esp32-sd.local` from home Wi-Fi hits `192.168.4.1` and times out.

Some Airtel / guest APs block client-to-client. Ping/HTTP to `192.168.1.29` then fails even though the ESP32 has an IP. Fix isolation, or use the hotspot.

`WiFi.setHostname("esp32-sd")` helps some routers show a DHCP name.

### FTP empty folder

Three separate bugs:

1. Library compiled as **FFat**, not SD — login worked, listing was empty. Web UI still showed files.
2. After joining home Wi-Fi, `ftpSrv.begin()` did not recreate sockets (`_listening` already true).
3. PASV advertised `192.168.4.1` to LAN clients, so LIST’s data channel never connected (looks empty).

Clipboard copy on `http://192.168.4.1` needs `document.execCommand("copy")`; `navigator.clipboard` is blocked on insecure origins.

### Video: one phone plays, another does not

Same x265 file. Phone Chrome often cannot decode HEVC/MKV. Native `<video>` shows “Unsupported video”.

FTP → VLC: some VLC builds seek badly over FTP (MP4 `moov` at end). Another phone’s VLC plays the same FTP file.

**HTTP Range** `http://<ip>/media/<file>.mp4` played on the phone that failed FTP. Play now uses that URL. Forcing `/v.mp4?path=` hid the real name and confused some VLC builds.

The ESP32 cannot transcode HEVC to H.264.

### Other

- Hidden file inputs failed on phones; Upload is a real `<label>` + drop zone.
- `HTTP_UPLOAD_BUFLEN` 8192; FAT/HTTP/FTP buffers 16 KB.
- FTP password length is limited by SimpleFTPServer `FTP_CRED_SIZE` (16). `sdreader1` fits.
- Uploading while browsing FTP can stall; close extra tabs.

## Limitations

- **Speed**: SPI + Wi-Fi, not USB 3 or gigabit NAS. Fine for browsing and VLC; large copies are slow.
- **Connections**: few TCP sockets. Close the web UI while using FTP; do not open many parallel downloads.
- **Codecs**: in-browser play ≈ H.264 MP4. HEVC/x265/MKV → VLC over **HTTP**.
- **Filesystem**: FAT32 only (Arduino `SD`). exFAT/NTFS will not mount. 4 GB max file size on FAT32.
- **Security**: open HTTP/FTP on the Wi-Fi. Default passwords. No TLS, no accounts, no write ACL.
- **Range**: ESP32 AP max 4 stations in `softAP(..., 4)`.
- **Not USB-SD**: the computer never sees a disk volume unless you use FTP/HTTP.
- **SDMMC**: not supported on this module.
- **Router isolation**: LAN access can be blocked no matter what firmware does.
- **Library patches** live outside the repo (`Documents/Arduino/libraries/SimpleFTPServer`). Reinstall = empty FTP again until `FtpServerKey.h` is STORAGE_SD.
- **Power**: USB from a weak port plus a 5 V SD module can brown out. Prefer a decent USB supply.

## Flash / serial

Board: `esp32:esp32:esp32`. If upload says “Wrong boot mode (0x13)”, retry; hold BOOT if needed.

Serial 115200 shows mount clock, root names, Wi-Fi IPs, and FTP start:

```
SD SPI mount OK at 16000000 Hz type=3 size=…
Joined Wi-Fi …  On that network open: http://192.168.1.29
FTP server started
  Hotspot: ftp://sd:sdreader1@192.168.4.1:21
  Home Wi-Fi: ftp://sd:sdreader1@192.168.1.29:21
```

## Defaults to keep

| Item | Value |
|------|--------|
| Hotspot | `ESP32-SD` / `sdreader1` |
| Web | `http://192.168.4.1` (hotspot) |
| FTP | `sd` / `sdreader1` port 21 |
| SPI | 5 / 18 / 19 / 23 @ 16 MHz when wires allow |
| mDNS | `esp32-sd.local` |
