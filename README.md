# ESP32 SD Reader

Wi-Fi NAS for a microSD card. Flash an ESP32, plug in a FAT32 card, and browse, copy, and play files from a phone or laptop. No USB computer required after flashing.

The web page lives in ESP32 flash, so the site still opens if the card is missing. You can fix wiring from Settings.

## What you need

- ESP32 DevKit (ESP32-D0WDQ6 or similar)
- 6-pin SPI microSD module (typical AMS1117 regulator)
- microSD card formatted **FAT32**
- USB cable to flash once
- Phone or laptop with Wi-Fi

## Wiring

Power the module from **VIN / 5V** if it has an AMS1117 regulator. Use 3V3 only for a 3.3V-only board.

| SD module | ESP32 |
|-----------|--------|
| CS / SS   | GPIO 5 |
| SCK / CLK | GPIO 18 |
| MISO / DO | GPIO 19 |
| MOSI / DI | GPIO 23 |
| VCC       | VIN / 5V |
| GND       | GND |

Typical cheap 6-pin modules **cannot** do native SDMMC. This firmware mounts **SPI only**.

## Flash

Arduino IDE or `arduino-cli`, board **ESP32 Dev Module** (`esp32:esp32:esp32`).

Libraries:

- ESP32 core (tested 3.3.11)
- [SimpleFTPServer](https://github.com/xreef/SimpleFTPServer) 3.0.2 — default storage must be **SD**, not FFat (see [info.md](info.md))

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings none .
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 .
```

Replace the serial port with yours.

## First connection (hotspot)

1. Power the ESP32.
2. Join Wi-Fi **ESP32-SD** / password **sdreader1**.
3. Open **http://192.168.4.1**

You can also try **http://esp32-sd.local** (mDNS; often fails on Android).

## Features

- Browse folders (list or grid)
- Sort by name, size, or type (folders / videos / images / other)
- Upload, download, delete, new folder
- Drag-and-drop upload
- **Play** opens the file over HTTP as `/media/filename` (best for VLC)
- **Watch** is a vertical swipe feed of videos in the folder (phone browser; H.264/MP4 works, HEVC often does not)
- Connect dialog with FTP / VLC / Wi-Fi QR codes
- Join home Wi-Fi (hotspot only, hotspot + home, or home only)
- If saved home Wi-Fi is missing after a reboot, the hotspot comes back automatically
- FTP server for FileZilla / Cyberduck
- HTTP byte-range streaming for VLC

## Home Wi-Fi

Settings → scan → pick your network → **Hotspot + join Wi-Fi**.

After it joins, the page shows a LAN address such as **http://192.168.1.29**.

| Your device Wi-Fi | Open this |
|-------------------|-----------|
| **ESP32-SD** (hotspot) | http://192.168.4.1 |
| Home / Airtel | the LAN IP on the Files page |

`http://192.168.4.1` does **not** work while you are on home Wi-Fi.

If the LAN page times out, the router is blocking device-to-device traffic (common on Airtel and guest networks). Turn off AP / client isolation, or stay on the hotspot.

## Play videos in VLC

Phone Chrome often cannot play **HEVC / x265**. Use VLC.

1. Stay on the same Wi-Fi as the ESP32.
2. Tap **Play** on a file, or in VLC: **Open Network Stream**.
3. Use the HTTP URL, for example:

```
http://192.168.1.29/media/VID_20251210_202010_964.mp4
```

On the hotspot, replace the IP with `192.168.4.1`.

HTTP seeking works. FTP playback of the same x265 file can fail on one phone and work on another — that is VLC on that phone, not a missing file.

## FTP

Use **FileZilla** or **Cyberduck**. Close the web page first (the ESP32 only handles a few connections).

| | Hotspot | Home Wi-Fi |
|---|---|---|
| Host | `192.168.4.1` | LAN IP (e.g. `192.168.1.29`) |
| Port | `21` | `21` |
| User | `sd` | `sd` |
| Password | `sdreader1` | `sdreader1` |
| Encryption | plain FTP | plain FTP |
| Transfer | passive | passive |

macOS Finder and the iOS Files app often show an empty folder. Do not use them.

## Settings you can change

- Hotspot name and password
- Wi-Fi mode and home network
- SPI pins (ESP32 DevKit, ESP32-S3 preset, or custom)
- Remount SD / reboot

## Default credentials

| | |
|---|---|
| Hotspot | `ESP32-SD` / `sdreader1` |
| Web page | no login |
| FTP | `sd` / `sdreader1` |

There is no HTTPS and no user accounts. Anyone on the Wi-Fi can read the card.

## More detail

Build notes, internals, bugs we hit, and limits: **[info.md](info.md)**
