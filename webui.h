#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#10140f">
<title>ESP32 SD Reader</title>
<style>
  :root {
    --bg: #10140f;
    --bg-2: #171c16;
    --card: #1d241c;
    --line: #2e382c;
    --text: #eef3e8;
    --muted: #9aa894;
    --accent: #d7b056;
    --accent-2: #8fd18a;
    --danger: #e06a6a;
    --shadow: 0 12px 40px rgba(0,0,0,.28);
    --radius: 16px;
  }
  * { box-sizing: border-box; }
  html, body { margin: 0; min-height: 100%; background: var(--bg); color: var(--text); }
  body {
    font-family: ui-sans-serif, system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    padding: 20px 16px 40px;
  }
  .wrap { max-width: 920px; margin: 0 auto; }
  header {
    display: flex; align-items: flex-start; justify-content: space-between; gap: 12px;
    margin-bottom: 18px;
  }
  .brand { display: flex; gap: 12px; align-items: center; min-width: 0; }
  .logo {
    width: 42px; height: 42px; border-radius: 12px; flex: none;
    background: linear-gradient(160deg, #ead07a, #b8872c);
    display: grid; place-items: center; color: #1a1608; font-weight: 800;
    box-shadow: var(--shadow);
  }
  h1 { font-size: 1.15rem; margin: 0; }
  .sub { color: var(--muted); font-size: .85rem; margin-top: 2px; }
  .chips { display: flex; flex-wrap: wrap; gap: 8px; justify-content: flex-end; align-items: center; }
  .chip {
    background: var(--card); border: 1px solid var(--line); border-radius: 999px;
    padding: 6px 10px; font-size: .75rem; color: var(--muted);
  }
  .chip b { color: var(--text); font-weight: 650; }
  .ok { color: var(--accent-2); }
  .bad { color: var(--danger); }
  nav {
    display: flex; gap: 8px; background: var(--bg-2); padding: 6px;
    border-radius: 14px; border: 1px solid var(--line); margin-bottom: 16px;
  }
  nav button {
    flex: 1; border: 0; background: transparent; color: var(--muted);
    padding: 10px 12px; border-radius: 10px; font-weight: 650; cursor: pointer;
  }
  nav button.active { background: var(--card); color: var(--text); }
  .panel {
    background: var(--card); border: 1px solid var(--line); border-radius: var(--radius);
    box-shadow: var(--shadow); overflow: hidden;
  }
  .toolbar {
    display: flex; flex-wrap: wrap; gap: 8px; align-items: center;
    padding: 12px; border-bottom: 1px solid var(--line);
  }
  .crumbs { flex: 1; min-width: 120px; display: flex; flex-wrap: wrap; gap: 4px; }
  .crumbs button {
    border: 0; background: transparent; color: var(--accent); cursor: pointer;
    padding: 4px 2px; font-weight: 650;
  }
  .crumbs span { color: var(--muted); }
  .view-toggle { display: flex; background: var(--bg-2); border: 1px solid var(--line); border-radius: 10px; overflow: hidden; }
  .view-toggle button { border: 0; background: transparent; color: var(--muted); padding: 8px 10px; cursor: pointer; font-weight: 650; }
  .view-toggle button.on { background: var(--accent); color: #1a1608; }
  .btn {
    border: 1px solid var(--line); background: var(--bg-2); color: var(--text);
    border-radius: 10px; padding: 8px 12px; cursor: pointer; font-weight: 650;
    display: inline-flex; align-items: center; gap: 6px; text-decoration: none; flex: none;
  }
  .btn.primary { background: var(--accent); color: #1a1608; border-color: transparent; }
  .btn.danger { color: var(--danger); }
  .btn:disabled { opacity: .5; cursor: not-allowed; }
  .link-row {
    display: flex; align-items: flex-start; gap: 10px;
    padding: 10px 12px; border: 1px solid var(--line); border-radius: 12px; background: var(--bg-2);
  }
  .link-row > span { font-size: .8rem; color: var(--muted); flex: none; padding-top: 2px; }
  .link-row code {
    flex: 1; min-width: 0; color: var(--accent); font-size: .82rem; line-height: 1.4;
    overflow-wrap: anywhere; word-break: break-all; white-space: normal;
    user-select: all; -webkit-user-select: all; cursor: text;
  }
  .link-row .btn { flex: none; margin-left: auto; align-self: center; }
  .drop {
    margin: 12px; border: 1.5px dashed var(--line); border-radius: 12px;
    padding: 12px 14px; color: var(--muted); font-size: .9rem; cursor: pointer;
    display: flex; align-items: center; justify-content: space-between; gap: 12px; flex-wrap: wrap;
  }
  .drop span { flex: 1; min-width: 160px; }
  .drop.hot { border-color: var(--accent); color: var(--text); background: rgba(215,176,86,.08); }
  .progress { height: 4px; background: var(--line); margin: 0 12px 6px; border-radius: 99px; overflow: hidden; display: none; }
  .progress > div { height: 100%; width: 0; background: var(--accent); }
  .xfer { margin: 0 12px 10px; font-size: .8rem; color: var(--muted); display: none; }
  .list {
    min-height: 180px;
    max-height: min(68vh, 720px);
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
    overscroll-behavior: contain;
  }
  .row {
    display: grid; grid-template-columns: 40px minmax(0, 1fr) auto; gap: 10px; align-items: center;
    padding: 12px 14px; border-top: 1px solid var(--line); cursor: pointer;
  }
  .row:hover { background: rgba(255,255,255,.03); }
  .icon {
    width: 36px; height: 36px; border-radius: 10px; display: grid; place-items: center;
    background: var(--bg-2); color: var(--accent); font-size: 16px;
  }
  .name { font-weight: 650; overflow-wrap: anywhere; word-break: break-word; line-height: 1.3; }
  .meta { color: var(--muted); font-size: .78rem; margin-top: 2px; }
  .acts {
    display: flex; gap: 6px; flex-wrap: nowrap; align-items: center;
    overflow-x: auto; -webkit-overflow-scrolling: touch;
    scrollbar-width: none; overscroll-behavior-x: contain;
  }
  .acts::-webkit-scrollbar { display: none; }
  .acts button { padding: 6px 8px; font-size: .78rem; flex: none; }
  .list.grid {
    display: grid; grid-template-columns: repeat(auto-fill, minmax(148px, 1fr));
    gap: 10px; padding: 12px;
  }
  .list.grid .row {
    display: flex; flex-direction: column; align-items: stretch; text-align: center;
    border: 1px solid var(--line); border-radius: 12px; padding: 12px 10px; gap: 8px;
  }
  .list.grid .icon { margin: 0 auto; width: 48px; height: 48px; font-size: 22px; }
  .list.grid .acts {
    justify-content: center; overflow: visible; flex-wrap: wrap;
  }
  #watchBtn[hidden] { display: none; }
  #feed {
    display: none; position: fixed; inset: 0; z-index: 40; background: #000;
  }
  #feed.on { display: block; }
  #feedTrack {
    height: 100%; height: 100dvh;
    overflow-y: auto; scroll-snap-type: y mandatory; scroll-behavior: smooth;
    -webkit-overflow-scrolling: touch; overscroll-behavior-y: contain;
  }
  .feed-slide {
    position: relative; height: 100%; height: 100dvh;
    scroll-snap-align: start; scroll-snap-stop: always;
    background: #000; display: flex; align-items: center; justify-content: center;
  }
  .feed-slide video {
    width: 100%; height: 100%; object-fit: contain; background: #000;
  }
  .feed-slide.fail video { visibility: hidden; }
  .feed-fail {
    display: none; position: absolute; inset: 0; z-index: 1;
    background: #111; color: #fff; place-items: center; text-align: center;
    padding: 28px 22px; gap: 12px;
  }
  .feed-slide.fail .feed-fail { display: grid; }
  .feed-fail b { font-size: 1.05rem; }
  .feed-fail .hint { color: rgba(255,255,255,.75); max-width: 22rem; }
  .feed-bar {
    position: absolute; top: 0; left: 0; right: 0; z-index: 2;
    display: flex; align-items: center; gap: 10px;
    padding: 12px 14px; padding-top: max(12px, env(safe-area-inset-top));
    background: linear-gradient(180deg, rgba(0,0,0,.65), transparent);
  }
  .feed-bar b {
    min-width: 0; flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;
    color: #fff; font-size: .9rem;
  }
  .feed-bar .btn { background: rgba(255,255,255,.14); color: #fff; border-color: transparent; }
  .feed-meta {
    position: absolute; left: 14px; right: 14px; bottom: 14px; z-index: 2;
    color: #fff; text-shadow: 0 1px 8px #000;
    padding-bottom: env(safe-area-inset-bottom);
  }
  .feed-meta .hint { color: rgba(255,255,255,.8); }
  .feed-progress {
    margin-top: 12px; height: 22px; display: flex; align-items: center; cursor: pointer;
  }
  .feed-progress-track {
    position: relative; width: 100%; height: 4px; border-radius: 99px;
    background: rgba(255,255,255,.22); overflow: hidden;
  }
  .feed-progress-buf, .feed-progress-now {
    position: absolute; left: 0; top: 0; bottom: 0; width: 0;
  }
  .feed-progress-buf { background: rgba(255,255,255,.38); }
  .feed-progress-now { background: var(--accent); }
  .feed-slide.fail .feed-progress { display: none; }
  .empty, .error { padding: 36px 16px; text-align: center; color: var(--muted); }
  .form { padding: 16px; display: grid; gap: 14px; }
  label { display: grid; gap: 6px; font-size: .85rem; color: var(--muted); }
  input, select {
    background: var(--bg); color: var(--text); border: 1px solid var(--line);
    border-radius: 10px; padding: 10px 12px; font: inherit;
  }
  .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
  .hint { font-size: .8rem; color: var(--muted); line-height: 1.45; }
  .mode-row { display: flex; flex-wrap: wrap; gap: 8px; }
  .mode-row label { display: flex; align-items: center; gap: 6px; padding: 8px 10px; border: 1px solid var(--line); border-radius: 10px; color: var(--text); }
  .net { display: flex; justify-content: space-between; gap: 8px; align-items: center; padding: 10px 12px; border-bottom: 1px solid var(--line); cursor: pointer; }
  .net:hover { background: rgba(255,255,255,.03); }
  .toast {
    position: fixed; left: 50%; bottom: 18px; transform: translateX(-50%);
    background: #111; color: #fff; padding: 10px 14px; border-radius: 999px;
    font-size: .85rem; display: none; z-index: 20; border: 1px solid var(--line);
    max-width: 90vw;
  }
  dialog {
    border: 1px solid var(--line); border-radius: 16px; background: var(--card); color: var(--text);
    width: min(840px, 96vw); padding: 0; max-height: 92vh;
  }
  dialog::backdrop { background: rgba(0,0,0,.55); }
  .preview-head { display: flex; align-items: center; padding: 12px 14px; border-bottom: 1px solid var(--line); gap: 10px; }
  .preview-head b { min-width: 0; flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .preview-head .btn { flex: none; }
  .preview-body { padding: 14px; max-height: calc(92vh - 56px); overflow: auto; display: grid; gap: 10px; }
  .preview-body img { max-width: 100%; border-radius: 10px; }
  .preview-body video {
    width: 100%; min-height: 220px; max-height: 50vh; border-radius: 10px;
    background: #000; object-fit: contain;
  }
  .preview-body pre { white-space: pre-wrap; word-break: break-word; font-size: .85rem; }
  .qr-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; }
  .qr-card {
    background: var(--bg-2); border: 1px solid var(--line); border-radius: 14px; padding: 12px; text-align: center;
  }
  .qr-card h3 { margin: 0 0 8px; font-size: .95rem; }
  .qr-card canvas { width: 100%; max-width: 180px; height: auto; background: #fff; border-radius: 8px; }
  .qr-card .kv { text-align: left; font-size: .78rem; color: var(--muted); margin-top: 8px; line-height: 1.5; }
  .qr-card .kv b { color: var(--text); }
  #fileInput { position: absolute; width: 1px; height: 1px; opacity: 0; overflow: hidden; }
  .upload-btn { position: relative; overflow: hidden; }
  @media (max-width: 720px) {
    .qr-grid { grid-template-columns: 1fr; }
    .grid-2 { grid-template-columns: 1fr; }
    header { flex-direction: column; }
    .chips { justify-content: flex-start; }
  }
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="brand">
      <div class="logo">SD</div>
      <div>
        <h1>ESP32 SD Reader</h1>
        <div class="sub">Connect to this hotspot, then browse the card in your browser.</div>
      </div>
    </div>
    <div class="chips">
      <button class="btn primary" type="button" id="connectBtn">Connect</button>
      <div class="chip">Used <b id="sdUsed">—</b></div>
      <div class="chip">SD <b id="sdState">...</b></div>
      <div class="chip">Clients <b id="clients">0</b></div>
    </div>
  </header>

  <nav>
    <button class="active" data-tab="files">Files</button>
    <button data-tab="settings">Settings</button>
  </nav>

  <section id="filesTab" class="panel">
    <div class="toolbar">
      <button class="btn" type="button" id="backBtn">Back</button>
      <div class="crumbs" id="crumbs"></div>
      <div class="view-toggle">
        <button type="button" id="listViewBtn" class="on">List</button>
        <button type="button" id="gridViewBtn">Grid</button>
      </div>
      <div class="view-toggle" id="sortToggle">
        <button type="button" data-sort="all" class="on">All</button>
        <button type="button" data-sort="name">Name</button>
        <button type="button" data-sort="size">Size</button>
      </div>
      <button class="btn primary" type="button" id="watchBtn" hidden>Watch</button>
      <button class="btn" type="button" id="mkdirBtn">New folder</button>
    </div>
    <div class="drop" id="drop">
      <span>Tap here or drop files to add them to this folder</span>
      <label class="btn primary upload-btn">Upload
        <input id="fileInput" type="file" multiple>
      </label>
    </div>
    <div class="progress" id="progress"><div></div></div>
    <div class="xfer" id="xfer"></div>
    <div class="list" id="list"><div class="empty">Loading…</div></div>
  </section>

  <section id="settingsTab" class="panel" hidden>
    <form class="form" id="settingsForm">
      <div class="hint">Hotspot page: <b>http://192.168.4.1</b> — only while your phone is on this ESP32 Wi-Fi. After joining home Wi-Fi, switch your phone to that network and open the LAN address (shown on the Files page). <b>http://192.168.4.1 will not work from home Wi-Fi.</b> If the LAN page times out, the router is blocking device-to-device (common on Airtel / guest Wi-Fi). Turn off AP isolation, or stay on the hotspot.</div>
      <div>
        <div class="hint" style="margin-bottom:8px">Wi-Fi mode</div>
        <div class="mode-row">
          <label><input type="radio" name="wifiMode" value="ap" checked> Hotspot only</label>
          <label><input type="radio" name="wifiMode" value="both"> Hotspot + join Wi-Fi</label>
          <label><input type="radio" name="wifiMode" value="sta"> Join Wi-Fi only</label>
        </div>
      </div>
      <label>Hotspot name (SSID)
        <input name="ssid" maxlength="31" required>
      </label>
      <label>Hotspot password (leave blank to keep the current one)
        <input name="password" maxlength="63" placeholder="********">
      </label>
      <label style="grid-template-columns:auto 1fr;align-items:center;gap:8px">
        <input name="open" type="checkbox" style="width:auto">
        Open hotspot (no password)
      </label>
      <div>
        <div class="hint" style="margin-bottom:8px">Join a Wi-Fi network</div>
        <div style="display:flex;gap:8px;flex-wrap:wrap;margin-bottom:8px">
          <button class="btn" type="button" id="scanBtn">Scan networks</button>
          <span class="hint" id="staStatus"></span>
        </div>
        <div id="netList" class="panel" style="max-height:220px;overflow:auto"></div>
        <div id="joinBox" hidden style="display:grid;gap:8px;margin-top:10px">
          <div class="hint">Network: <b id="joinSsid"></b></div>
          <input id="joinPass" type="password" placeholder="Wi-Fi password">
          <button class="btn primary" type="button" id="joinBtn">Join network</button>
        </div>
      </div>
      <label>Board preset
        <select id="preset">
          <option value="esp32">ESP32 DevKit — CS 5, SCK 18, MISO 19, MOSI 23</option>
          <option value="s3">ESP32-S3 DevKit — CS 10, SCK 12, MISO 13, MOSI 11</option>
          <option value="custom">Custom pins</option>
        </select>
      </label>
      <div class="grid-2">
        <label>CS / SS<input name="cs" type="number" min="0" max="48" required></label>
        <label>SCK<input name="sck" type="number" min="0" max="48" required></label>
        <label>MISO<input name="miso" type="number" min="0" max="48" required></label>
        <label>MOSI<input name="mosi" type="number" min="0" max="48" required></label>
      </div>
      <div class="hint">This board uses the SPI pins above. Typical 6-pin modules cannot do native SDMMC.</div>
      <div class="hint" id="sdHint"></div>
      <div style="display:flex;gap:8px;flex-wrap:wrap">
        <button class="btn primary" type="submit">Save settings</button>
        <button class="btn" type="button" id="remountBtn">Remount SD</button>
        <button class="btn" type="button" id="rebootBtn">Reboot ESP32</button>
      </div>
    </form>
  </section>
</div>

<dialog id="preview">
  <div class="preview-head">
    <b id="previewName">Preview</b>
    <button class="btn" id="closePreview">Close</button>
  </div>
  <div class="preview-body" id="previewBody"></div>
</dialog>

<dialog id="connectDlg">
  <div class="preview-head">
    <b>Connect</b>
    <button class="btn" id="closeConnect">Close</button>
  </div>
  <div class="preview-body">
    <div class="link-row" id="lanRow" hidden>
      <span>Home Wi-Fi</span>
      <code id="lanLink">http://</code>
      <button class="btn" type="button" id="copyLanBtn">Copy</button>
    </div>
    <div class="link-row">
      <span>FTP</span>
      <code id="ftpLink">ftp://192.168.4.1:21</code>
      <button class="btn" type="button" id="copyFtpBtn">Copy</button>
    </div>
    <div class="hint">Use FileZilla or Cyberduck. Close this web page first. Connect to the same Wi-Fi you will use for FTP, then use that network’s IP (hotspot <b>192.168.4.1</b> or the home Wi-Fi address). Finder and the iOS Files app often fail.</div>
    <div class="link-row">
      <span>VLC HTTP</span>
      <code id="vlcLink">http://192.168.4.1/media/</code>
      <button class="btn" type="button" id="copyVlcBtn">Copy</button>
    </div>
      <div class="hint">For x265 / HEVC use VLC → Open Network Stream and paste the file’s HTTP link (not FTP). Some phones can decode HEVC over FTP and others cannot — that is the phone’s VLC, not the card.</div>
    <div class="qr-grid">
      <div class="qr-card">
        <h3>Wi-Fi</h3>
        <canvas id="qrWifi" width="180" height="180"></canvas>
        <div class="kv">Name: <b id="wifiSsid">—</b><br>Password: <b id="wifiPass">—</b></div>
        <button class="btn" type="button" id="copyWifiBtn" style="margin-top:8px">Copy password</button>
      </div>
      <div class="qr-card">
        <h3>Web page</h3>
        <canvas id="qrWeb" width="180" height="180"></canvas>
        <div class="kv"><b id="webUrl">http://192.168.4.1</b></div>
        <button class="btn" type="button" id="copyWebBtn" style="margin-top:8px">Copy link</button>
      </div>
      <div class="qr-card">
        <h3>FTP</h3>
        <canvas id="qrFtp" width="180" height="180"></canvas>
        <div class="kv">Host: <b id="ftpHost">192.168.4.1</b><br>User: <b id="ftpUser">sd</b><br>Password: <b id="ftpPass">sdreader1</b><br>Port: <b>21</b></div>
        <button class="btn" type="button" id="copyFtpDlgBtn" style="margin-top:8px">Copy FTP link</button>
      </div>
    </div>
  </div>
</dialog>
<div class="toast" id="toast"></div>
<div id="feed">
  <div id="feedTrack"></div>
</div>

<script>
const $ = (s) => document.querySelector(s);
let path = "/";
let statusData = {};
let viewMode = localStorage.getItem("sdView") || "list";
let joinTarget = "";
let folderItems = [];
let feedObserver = null;
let feedMuted = true;
let sortBy = localStorage.getItem("sdSort") || "all";
let sortAsc = localStorage.getItem("sdSortAsc") !== "0";

const TABS = document.querySelectorAll("nav button");
TABS.forEach((b) => b.onclick = () => {
  TABS.forEach((x) => x.classList.toggle("active", x === b));
  $("#filesTab").hidden = b.dataset.tab !== "files";
  $("#settingsTab").hidden = b.dataset.tab !== "settings";
});

function toast(msg) {
  const t = $("#toast");
  t.textContent = msg;
  t.style.display = "block";
  clearTimeout(toast._t);
  toast._t = setTimeout(() => t.style.display = "none", 2800);
}

function fmtSize(n) {
  if (n == null || n < 0) return "—";
  const u = ["B","KB","MB","GB"];
  let i = 0, v = n;
  while (v >= 1024 && i < u.length - 1) { v /= 1024; i++; }
  return v.toFixed(i ? 1 : 0) + " " + u[i];
}

function iconFor(item) {
  if (item.dir) return "📁";
  const n = item.name.toLowerCase();
  if (/\.(png|jpg|jpeg|gif|webp|bmp)$/.test(n)) return "🖼️";
  if (/\.(txt|log|md|json|csv|ini)$/.test(n)) return "📄";
  if (/\.(mp3|wav|ogg)$/.test(n)) return "🎵";
  if (/\.(mp4|m4v|webm|mkv|mov|avi)$/.test(n)) return "🎬";
  if (/\.(pdf)$/.test(n)) return "📕";
  return "📦";
}

function joinPath(base, name) {
  if (base === "/") return "/" + name;
  return base + "/" + name;
}

function setView(mode) {
  viewMode = mode;
  localStorage.setItem("sdView", mode);
  $("#listViewBtn").classList.toggle("on", mode === "list");
  $("#gridViewBtn").classList.toggle("on", mode === "grid");
  $("#list").classList.toggle("grid", mode === "grid");
}

function goBack() {
  if (path === "/") return;
  const parts = path.split("/").filter(Boolean);
  parts.pop();
  path = parts.length ? "/" + parts.join("/") : "/";
  loadFiles();
}

function crumbs() {
  const el = $("#crumbs");
  el.innerHTML = "";
  const parts = path.split("/").filter(Boolean);
  const add = (label, p) => {
    const b = document.createElement("button");
    b.type = "button";
    b.textContent = label;
    b.onclick = () => { path = p; loadFiles(); };
    el.appendChild(b);
  };
  add("SD", "/");
  let cur = "";
  parts.forEach((part) => {
    const s = document.createElement("span");
    s.textContent = "/";
    el.appendChild(s);
    cur += "/" + part;
    add(part, cur);
  });
  const back = $("#backBtn");
  if (back) back.disabled = path === "/";
}

function itemKind(item) {
  if (item.dir) return 0;
  if (isVideo(item.name)) return 1;
  const n = item.name.toLowerCase();
  if (/\.(png|jpg|jpeg|gif|webp|bmp)$/.test(n)) return 2;
  if (/\.(mp3|wav|ogg)$/.test(n)) return 3;
  return 4;
}

function sortedItems() {
  const items = folderItems.slice();
  const dir = sortAsc ? 1 : -1;
  items.sort((a, b) => {
    if (sortBy === "size") {
      const as = a.dir ? -1 : (a.size || 0);
      const bs = b.dir ? -1 : (b.size || 0);
      if (as !== bs) return (as - bs) * dir;
    } else if (sortBy === "all") {
      const ka = itemKind(a);
      const kb = itemKind(b);
      if (ka !== kb) return (ka - kb) * dir;
    }
    return displayName(a.name).localeCompare(displayName(b.name), undefined, { numeric: true, sensitivity: "base" }) * dir;
  });
  return items;
}

function syncSort() {
  document.querySelectorAll("#sortToggle [data-sort]").forEach((b) => {
    b.classList.toggle("on", b.dataset.sort === sortBy);
  });
}

function setSort(next) {
  if (sortBy === next) sortAsc = !sortAsc;
  else { sortBy = next; sortAsc = true; }
  localStorage.setItem("sdSort", sortBy);
  localStorage.setItem("sdSortAsc", sortAsc ? "1" : "0");
  syncSort();
  renderFiles();
}

function ftpUri() {
  const host = location.hostname || statusData.ip || "192.168.4.1";
  const user = statusData.ftpUser || "sd";
  const pass = statusData.ftpPass || "sdreader1";
  return "ftp://" + user + ":" + pass + "@" + host + ":21";
}

function ftpFileUrlIn(dir, name) {
  const parts = joinPath(dir, name).split("/").filter(Boolean).map((p) => encodeURI(p));
  return ftpUri() + "/" + parts.join("/");
}

function ftpFileUrl(name) {
  return ftpFileUrlIn(path, name);
}

function vlcHttpUrlIn(dir, name) {
  const parts = joinPath(dir, name).split("/").filter(Boolean).map(encodeURIComponent);
  return location.origin + "/media/" + parts.join("/");
}

function openUrl(url) {
  const a = document.createElement("a");
  a.href = url;
  a.rel = "noopener";
  document.body.appendChild(a);
  a.click();
  a.remove();
}

function playHttp(dir, name) {
  const http = vlcHttpUrlIn(dir, name);
  const app = vlcAppLink(http);
  openUrl(app !== http ? app : http);
}

function folderVideos(items) {
  return (items || []).filter((i) => !i.dir && isVideo(i.name));
}

function closeFeed() {
  if (feedObserver) { feedObserver.disconnect(); feedObserver = null; }
  $("#feedTrack").querySelectorAll("video").forEach((v) => {
    v.pause();
    v.removeAttribute("src");
    v.load();
  });
  $("#feedTrack").innerHTML = "";
  $("#feed").classList.remove("on");
}

function activateSlide(slide) {
  const track = $("#feedTrack");
  track.querySelectorAll(".feed-slide").forEach((s) => {
    const vid = s.querySelector("video");
    if (!vid) return;
    if (s === slide) {
      if (s.classList.contains("fail")) return;
      if (!vid.getAttribute("src")) {
        setTimeout(() => {
          if (!vid.getAttribute("src")) vid.src = vid.dataset.src;
          vid.muted = feedMuted;
          const p = vid.play();
          if (p && p.catch) p.catch(() => {});
        }, 120);
      } else {
        vid.muted = feedMuted;
        const p = vid.play();
        if (p && p.catch) p.catch(() => {});
      }
    } else {
      vid.pause();
      if (vid.getAttribute("src")) {
        vid.removeAttribute("src");
        vid.load();
      }
    }
  });
  const muteBtn = slide.querySelector("[data-mute]");
  if (muteBtn) muteBtn.textContent = feedMuted ? "Unmute" : "Mute";
}

function updateFeedProgress(slide, video) {
  const now = slide.querySelector(".feed-progress-now");
  const buf = slide.querySelector(".feed-progress-buf");
  if (!now || !video.duration) return;
  now.style.width = Math.min(100, (video.currentTime / video.duration) * 100) + "%";
  if (buf && video.buffered.length) {
    buf.style.width = Math.min(100, (video.buffered.end(video.buffered.length - 1) / video.duration) * 100) + "%";
  }
}

function markFeedFail(slide, video) {
  slide.classList.add("fail");
  video.pause();
}

function openFeed(videos, startName, dir) {
  if (!videos.length) {
    toast("No videos in this folder");
    return;
  }
  closeFeed();
  const track = $("#feedTrack");
  const start = Math.max(0, videos.findIndex((v) => v.name === startName));
  videos.forEach((item, i) => {
    const slide = document.createElement("div");
    slide.className = "feed-slide";
    const src = vlcHttpUrlIn(dir, item.name);
    slide.innerHTML =
      '<div class="feed-bar">' +
        '<b>' + escapeHtml(displayName(item.name)) + '</b>' +
        '<button class="btn" type="button" data-mute>Unmute</button>' +
        '<button class="btn" type="button" data-close>Close</button>' +
      '</div>' +
      '<video playsinline webkit-playsinline loop preload="none"></video>' +
      '<div class="feed-fail">' +
        '<b>Unsupported video</b>' +
        '<div class="hint">This browser cannot play HEVC / x265. Use VLC. If one phone plays it and this one does not, update VLC or use HTTP Open Network Stream (not FTP).</div>' +
        '<a class="btn primary" data-vlc>Play on VLC</a>' +
        '<a class="btn" data-vlc-ftp>VLC via FTP</a>' +
      '</div>' +
      '<div class="feed-meta"><div>' + (i + 1) + " / " + videos.length +
      '</div><div class="hint">Swipe up for the next video</div>' +
      '<div class="feed-progress"><div class="feed-progress-track">' +
      '<div class="feed-progress-buf"></div><div class="feed-progress-now"></div>' +
      '</div></div></div>';
    const video = slide.querySelector("video");
    video.dataset.src = src;
    video.muted = true;
    slide.querySelector("[data-vlc]").href = vlcAppLink(src);
    slide.querySelector("[data-vlc-ftp]").href = vlcAppLink(ftpFileUrlIn(dir, item.name));
    video.onerror = () => markFeedFail(slide, video);
    video.onplaying = () => slide.classList.remove("fail");
    video.ontimeupdate = () => updateFeedProgress(slide, video);
    video.onprogress = () => updateFeedProgress(slide, video);
    video.onclick = () => {
      if (slide.classList.contains("fail")) return;
      if (video.paused) video.play().catch(() => {});
      else video.pause();
    };
    slide.querySelector(".feed-progress").onclick = (e) => {
      e.stopPropagation();
      if (!video.duration) return;
      const trackEl = e.currentTarget.querySelector(".feed-progress-track");
      const r = trackEl.getBoundingClientRect();
      video.currentTime = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)) * video.duration;
    };
    slide.querySelector("[data-mute]").onclick = (e) => {
      e.stopPropagation();
      feedMuted = !feedMuted;
      video.muted = feedMuted;
      track.querySelectorAll("[data-mute]").forEach((b) => b.textContent = feedMuted ? "Unmute" : "Mute");
    };
    slide.querySelector("[data-close]").onclick = (e) => { e.stopPropagation(); closeFeed(); };
    track.appendChild(slide);
  });
  $("#feed").classList.add("on");
  feedObserver = new IntersectionObserver((entries) => {
    entries.forEach((en) => {
      if (en.isIntersecting && en.intersectionRatio >= 0.6) activateSlide(en.target);
    });
  }, { root: track, threshold: [0.6] });
  track.querySelectorAll(".feed-slide").forEach((s) => feedObserver.observe(s));
  const first = track.children[start] || track.children[0];
  if (first) {
    first.scrollIntoView();
    activateSlide(first);
  }
}

async function watchPath(dir, startName) {
  if (dir === path) {
    openFeed(folderVideos(sortedItems()), startName, dir);
    return;
  }
  try {
    const r = await fetch("/api/list?path=" + encodeURIComponent(dir));
    const data = await r.json();
    if (!r.ok) throw new Error(data.error || "Could not list folder");
    openFeed(folderVideos(data.items), startName, dir);
  } catch (e) {
    toast(e.message);
  }
}

function drawQr(canvas, size, bits) {
  const ctx = canvas.getContext("2d");
  const quiet = 2;
  const dim = size + quiet * 2;
  canvas.width = dim * 8;
  canvas.height = dim * 8;
  const s = canvas.width / dim;
  ctx.fillStyle = "#fff";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = "#111";
  for (let i = 0; i < bits.length; i++) {
    if (bits[i] !== "1") continue;
    const x = i % size;
    const y = Math.floor(i / size);
    ctx.fillRect((x + quiet) * s, (y + quiet) * s, s, s);
  }
}

async function loadQr(kind, canvas) {
  const r = await fetch("/api/qr?kind=" + kind);
  const data = await r.json();
  if (!r.ok) throw new Error(data.error || "QR failed");
  drawQr(canvas, data.size, data.bits);
}

async function openConnect() {
  const ip = statusData.ip || "192.168.4.1";
  $("#wifiSsid").textContent = statusData.ssid || "ESP32-SD";
  $("#wifiPass").textContent = statusData.wifiPass || "(open)";
  $("#webUrl").textContent = statusData.web || ("http://" + ip);
  $("#ftpHost").textContent = location.hostname || ip;
  $("#ftpUser").textContent = statusData.ftpUser || "sd";
  $("#ftpPass").textContent = statusData.ftpPass || "sdreader1";
  $("#ftpLink").textContent = ftpUri();
  $("#vlcLink").textContent = (location.origin || ("http://" + (statusData.staIp || ip))) + "/media/";
  if (statusData.staConnected && statusData.staIp) {
    $("#lanRow").hidden = false;
    $("#lanLink").textContent = statusData.webLan || ("http://" + statusData.staIp);
  } else {
    $("#lanRow").hidden = true;
  }
  $("#connectDlg").showModal();
  try {
    await Promise.all([
      loadQr("wifi", $("#qrWifi")),
      loadQr("web", $("#qrWeb")),
      loadQr("ftp", $("#qrFtp"))
    ]);
  } catch (e) {
    toast(e.message);
  }
}

function copyText(text, okMsg) {
  const ta = document.createElement("textarea");
  ta.value = text;
  ta.setAttribute("readonly", "");
  ta.style.cssText = "position:fixed;top:12px;left:12px;width:2em;height:2em;opacity:0;z-index:99";
  document.body.appendChild(ta);
  ta.focus();
  ta.select();
  ta.setSelectionRange(0, text.length);
  let ok = false;
  try { ok = document.execCommand("copy"); } catch (e) {}
  document.body.removeChild(ta);
  if (ok) {
    toast(okMsg);
    return;
  }
  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(text).then(() => toast(okMsg)).catch(() => {
      toast("Copy failed. Long-press the yellow link and copy.");
    });
    return;
  }
  toast("Copy failed. Long-press the yellow link and copy.");
}

async function loadStatus() {
  const r = await fetch("/api/status");
  statusData = await r.json();
  const ok = !!statusData.sd;
  const st = $("#sdState");
  st.textContent = ok ? "mounted" : "not found";
  st.className = ok ? "ok" : "bad";
  $("#sdUsed").textContent = ok ? (fmtSize(statusData.used) + " / " + fmtSize(statusData.total)) : "—";
  $("#clients").textContent = statusData.clients ?? 0;
  $("#sdHint").textContent = ok
    ? ("Card type " + (statusData.cardType || "SD") + " · " + (statusData.sdBus || "SPI") + " " + ((statusData.sdHz||0)/1000000).toFixed(1) + " MHz · uptime " + Math.floor((statusData.uptime || 0)/1000) + "s · free heap " + fmtSize(statusData.heap))
    : "SD card did not mount. Check wiring, VIN/5V, and FAT32.";
  const sta = $("#staStatus");
  const sub = document.querySelector(".sub");
  if (statusData.hotspotFallback) {
    sta.textContent = "Home Wi-Fi \"" + (statusData.staSsid || "") + "\" not found — hotspot is on";
    if (sub) sub.textContent = "Home Wi-Fi not found. Connect to hotspot " + (statusData.ssid || "ESP32-SD") + " then open http://192.168.4.1";
  } else if (statusData.staConnected) {
    sta.textContent = "Joined " + statusData.staSsid + " · " + statusData.staIp;
    if (sub) sub.textContent = "On \"" + statusData.staSsid + "\" open http://" + statusData.staIp + "  ·  Hotspot stays at http://192.168.4.1";
  } else if (statusData.staSsid) {
    sta.textContent = "Not connected to " + statusData.staSsid;
    if (sub) sub.textContent = "Connect to this hotspot, then browse the card in your browser.";
  } else {
    sta.textContent = "";
    if (sub) sub.textContent = "Connect to this hotspot, then browse the card in your browser.";
  }
}

async function loadSettings() {
  const r = await fetch("/api/settings");
  const s = await r.json();
  const f = $("#settingsForm");
  f.ssid.value = s.ssid || "";
  f.password.value = "";
  f.password.placeholder = s.hasPassword ? "unchanged unless you type a new one" : "open network";
  f.cs.value = s.cs;
  f.sck.value = s.sck;
  f.miso.value = s.miso;
  f.mosi.value = s.mosi;
  const mode = s.wifiMode || "ap";
  const radio = f.querySelector('input[name="wifiMode"][value="' + mode + '"]');
  if (radio) radio.checked = true;
  syncPreset();
}

function syncPreset() {
  const f = $("#settingsForm");
  const p = $("#preset").value;
  const custom = p === "custom";
  ["cs","sck","miso","mosi"].forEach((n) => f[n].readOnly = !custom);
  if (p === "esp32") { f.cs.value = 5; f.sck.value = 18; f.miso.value = 19; f.mosi.value = 23; }
  if (p === "s3") { f.cs.value = 10; f.sck.value = 12; f.miso.value = 13; f.mosi.value = 11; }
}

function detectPreset(s) {
  if (s.cs == 5 && s.sck == 18 && s.miso == 19 && s.mosi == 23) return "esp32";
  if (s.cs == 10 && s.sck == 12 && s.miso == 13 && s.mosi == 11) return "s3";
  return "custom";
}

async function loadFiles() {
  crumbs();
  const box = $("#list");
  box.classList.toggle("grid", viewMode === "grid");
    box.innerHTML = '<div class="empty">Loading…</div>';
    $("#watchBtn").hidden = true;
  try {
    const r = await fetch("/api/list?path=" + encodeURIComponent(path));
    const data = await r.json();
    if (!r.ok) throw new Error(data.error || "Could not list files");
    if (!data.items.length) {
      folderItems = [];
      $("#watchBtn").hidden = true;
      box.innerHTML = '<div class="empty">This folder is empty.</div>';
      return;
    }
    folderItems = data.items;
    const vids = folderVideos(folderItems);
    $("#watchBtn").hidden = vids.length === 0;
    renderFiles();
  } catch (e) {
    box.innerHTML = '<div class="error">' + escapeHtml(e.message) + '</div>';
  }
}

function renderFiles() {
  const box = $("#list");
  box.classList.toggle("grid", viewMode === "grid");
  const items = sortedItems();
  if (!items.length) {
    box.innerHTML = '<div class="empty">This folder is empty.</div>';
    return;
  }
  box.innerHTML = "";
  items.forEach((item) => {
      const row = document.createElement("div");
      row.className = "row";
      row.innerHTML = `
        <div class="icon">${iconFor(item)}</div>
        <div>
          <div class="name">${escapeHtml(displayName(item.name))}</div>
          <div class="meta">${item.dir ? "Folder" : fmtSize(item.size)}</div>
        </div>
        <div class="acts"></div>`;
      const acts = row.querySelector(".acts");
      acts.onclick = (e) => e.stopPropagation();
      if (item.dir) {
        const watch = document.createElement("button");
        watch.className = "btn primary";
        watch.textContent = "Watch";
        watch.onclick = (e) => { e.stopPropagation(); watchPath(joinPath(path, item.name)); };
        acts.appendChild(watch);
      } else if (isVideo(item.name)) {
        const play = document.createElement("button");
        play.className = "btn primary";
        play.textContent = "Play";
        play.onclick = (e) => { e.stopPropagation(); preview(item); };
        acts.appendChild(play);
      }
      if (!item.dir) {
        const dl = document.createElement("button");
        dl.className = "btn";
        dl.textContent = "Save";
        dl.onclick = (e) => { e.stopPropagation(); download(item.name); };
        acts.appendChild(dl);
      }
      const del = document.createElement("button");
      del.className = "btn danger";
      del.textContent = "Delete";
      del.onclick = (e) => { e.stopPropagation(); removeItem(item); };
      acts.appendChild(del);
      row.onclick = () => {
        if (item.dir) openDir(item.name);
        else preview(item);
      };
      box.appendChild(row);
  });
}

function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

function displayName(name) {
  let s = String(name);
  for (let i = 0; i < 3; i++) {
    try {
      const d = decodeURIComponent(s.replace(/\+/g, " "));
      if (d === s) break;
      s = d;
    } catch (e) { break; }
  }
  return s;
}

function openDir(name) {
  path = joinPath(path, name);
  loadFiles();
}

function isVideo(name) {
  return /\.(mp4|m4v|webm|mkv|mov|avi)$/i.test(name);
}

function fileUrl(name, dl) {
  const p = joinPath(path, name);
  return "/api/file?path=" + encodeURIComponent(p) + (dl ? "&download=1" : "");
}

function mediaUrl(name) {
  const parts = joinPath(path, name).split("/").filter(Boolean).map(encodeURIComponent);
  return location.origin + "/media/" + parts.join("/");
}

function videoMime(name) {
  const n = name.toLowerCase();
  if (/\.(mp4|m4v)$/.test(n)) return "video/mp4";
  if (/\.webm$/.test(n)) return "video/webm";
  if (/\.mov$/.test(n)) return "video/quicktime";
  if (/\.mkv$/.test(n)) return "video/x-matroska";
  if (/\.avi$/.test(n)) return "video/x-msvideo";
  return "";
}

function vlcAppLink(url) {
  const ua = navigator.userAgent || "";
  if (/iPhone|iPad|iPod/i.test(ua)) {
    return "vlc-x-callback://x-callback-url/stream?url=" + encodeURIComponent(url);
  }
  if (/Android/i.test(ua)) {
    const ftp = url.startsWith("ftp://");
    const rest = url.replace(/^https?:\/\//, "").replace(/^ftp:\/\//, "");
    const mime = videoMime(url) || "video/*";
    return "intent://" + rest + "#Intent;scheme=" + (ftp ? "ftp" : "http") +
      ";action=android.intent.action.VIEW;type=" + mime +
      ";package=org.videolan.vlc;S.browser_fallback_url=https://play.google.com/store/apps/details?id=org.videolan.vlc;end";
  }
  return url;
}

function download(name) {
  const a = document.createElement("a");
  a.href = fileUrl(name, true);
  a.download = name;
  a.click();
}

async function preview(item) {
  const dlg = $("#preview");
  $("#previewName").textContent = displayName(item.name);
  const body = $("#previewBody");
  const n = item.name.toLowerCase();
  if (isVideo(item.name)) {
    const http = vlcHttpUrlIn(path, item.name);
    const ftp = ftpFileUrl(item.name);
    const mime = videoMime(item.name);
    const typeAttr = mime ? ' type="' + mime + '"' : "";
    body.innerHTML =
      '<video id="player" controls playsinline webkit-playsinline preload="none">' +
      '<source src="' + escapeHtml(http) + '"' + typeAttr + ">" +
      "</video>" +
      '<a class="btn primary" id="openVlcBtn">Open in VLC</a>' +
      '<div class="hint">x265 / HEVC often fails in the phone browser. In VLC use <b>Open Network Stream</b> and paste the HTTP link. HTTP seeking works better than FTP on some phones.</div>' +
      '<div class="link-row"><span>HTTP</span><code id="httpCopy">' + escapeHtml(http) + '</code><button class="btn" type="button" id="copyVlcHttp">Copy</button></div>' +
      '<div class="link-row"><span>FTP</span><code id="ftpCopy">' + escapeHtml(ftp) + '</code><button class="btn" type="button" id="copyVlcFtp">Copy</button></div>';
    $("#openVlcBtn").href = vlcAppLink(http);
    $("#copyVlcHttp").onclick = (e) => { e.preventDefault(); copyText(http, "HTTP link copied"); };
    $("#copyVlcFtp").onclick = (e) => { e.preventDefault(); copyText(ftp, "FTP link copied"); };
    $("#httpCopy").onclick = () => copyText(http, "HTTP link copied");
    $("#ftpCopy").onclick = () => copyText(ftp, "FTP link copied");
  } else if (/\.(png|jpg|jpeg|gif|webp|bmp)$/.test(n)) {
    body.innerHTML = '<img alt="" src="' + fileUrl(item.name, false) + '">';
  } else if (/\.(txt|log|md|json|csv|ini|html|css|js)$/.test(n) && item.size < 200000) {
    body.innerHTML = "<pre>Loading…</pre>";
    const r = await fetch(fileUrl(item.name, false));
    const t = await r.text();
    body.innerHTML = "<pre></pre>";
    body.firstChild.textContent = t;
  } else {
    body.innerHTML = '<div class="hint">No in-browser preview for this type. Use Save to download it.</div>';
  }
  dlg.showModal();
}

async function removeItem(item) {
  if (!confirm("Delete " + item.name + "?")) return;
  const r = await fetch("/api/delete", {
    method: "POST",
    headers: {"Content-Type": "application/x-www-form-urlencoded"},
    body: "path=" + encodeURIComponent(joinPath(path, item.name))
  });
  const data = await r.json();
  if (!r.ok) return toast(data.error || "Delete failed");
  toast("Deleted");
  loadFiles(); loadStatus();
}

$("#mkdirBtn").onclick = async () => {
  const name = prompt("Folder name");
  if (!name) return;
  const r = await fetch("/api/mkdir", {
    method: "POST",
    headers: {"Content-Type": "application/x-www-form-urlencoded"},
    body: "path=" + encodeURIComponent(joinPath(path, name))
  });
  const data = await r.json();
  if (!r.ok) return toast(data.error || "Could not create folder");
  loadFiles();
};

function uploadFiles(files) {
  if (!files || !files.length) return;
  const fd = new FormData();
  for (const f of files) fd.append("file", f, f.name);
  const bar = $("#progress");
  const inner = bar.firstElementChild;
  const xfer = $("#xfer");
  bar.style.display = "block";
  xfer.style.display = "block";
  inner.style.width = "0%";
  xfer.textContent = "Starting…";
  const started = Date.now();
  let lastLoaded = 0;
  let lastT = started;
  const xhr = new XMLHttpRequest();
  xhr.upload.onprogress = (e) => {
    if (!e.lengthComputable) return;
    const now = Date.now();
    const pct = Math.round(e.loaded / e.total * 100);
    inner.style.width = pct + "%";
    const inst = (e.loaded - lastLoaded) / Math.max(0.25, (now - lastT) / 1000);
    const avg = e.loaded / Math.max(0.25, (now - started) / 1000);
    lastLoaded = e.loaded;
    lastT = now;
    const left = avg > 0 ? Math.ceil((e.total - e.loaded) / avg) : 0;
    xfer.textContent = pct + "% · " + fmtSize(inst) + "/s · " + fmtSize(e.loaded) + " / " + fmtSize(e.total) + " · " + left + "s left";
  };
  xhr.onload = () => {
    bar.style.display = "none";
    xfer.style.display = "none";
    let msg = "Uploaded";
    if (xhr.status !== 200) {
      try { msg = JSON.parse(xhr.responseText).error || "Upload failed"; }
      catch (e) { msg = "Upload failed (" + xhr.status + ")"; }
    }
    toast(msg);
    loadFiles(); loadStatus();
  };
  xhr.onerror = () => { bar.style.display = "none"; xfer.style.display = "none"; toast("Upload failed"); };
  xhr.open("POST", "/api/upload?path=" + encodeURIComponent(path));
  xhr.send(fd);
}

$("#fileInput").onchange = (e) => { uploadFiles([...e.target.files]); e.target.value = ""; };
const drop = $("#drop");
drop.onclick = (e) => {
  if (e.target.closest(".upload-btn") || e.target.id === "fileInput") return;
  $("#fileInput").click();
};
["dragenter","dragover"].forEach((ev) => drop.addEventListener(ev, (e) => { e.preventDefault(); drop.classList.add("hot"); }));
["dragleave","drop"].forEach((ev) => drop.addEventListener(ev, (e) => { e.preventDefault(); drop.classList.remove("hot"); }));
drop.addEventListener("drop", (e) => uploadFiles([...e.dataTransfer.files]));

$("#backBtn").onclick = goBack;
$("#listViewBtn").onclick = () => { setView("list"); renderFiles(); };
$("#gridViewBtn").onclick = () => { setView("grid"); renderFiles(); };
document.querySelectorAll("#sortToggle [data-sort]").forEach((b) => {
  b.onclick = () => setSort(b.dataset.sort);
});
$("#watchBtn").onclick = () => watchPath(path);
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape" && $("#feed").classList.contains("on")) closeFeed();
});
$("#connectBtn").onclick = openConnect;
$("#closeConnect").onclick = () => $("#connectDlg").close();
$("#copyFtpBtn").onclick = () => copyText($("#ftpLink").textContent, "FTP link copied");
$("#copyLanBtn").onclick = () => copyText($("#lanLink").textContent, "Home Wi-Fi link copied");
$("#copyVlcBtn").onclick = () => copyText($("#vlcLink").textContent, "VLC HTTP copied");
$("#copyFtpDlgBtn").onclick = () => copyText(ftpUri(), "FTP link copied");
$("#copyWifiBtn").onclick = () => copyText(statusData.wifiPass || "", "Password copied");
$("#copyWebBtn").onclick = () => copyText(statusData.web || "http://192.168.4.1", "Web link copied");

$("#scanBtn").onclick = async () => {
  $("#netList").innerHTML = '<div class="empty">Scanning…</div>';
  const r = await fetch("/api/scan");
  const nets = await r.json();
  if (!nets.length) {
    $("#netList").innerHTML = '<div class="empty">No networks found.</div>';
    return;
  }
  $("#netList").innerHTML = "";
  nets.forEach((n) => {
    const row = document.createElement("div");
    row.className = "net";
    row.innerHTML = "<div><b>" + escapeHtml(n.ssid) + "</b><div class='meta'>" + n.rssi + " dBm" + (n.secure ? " · locked" : " · open") + "</div></div><button class='btn' type='button'>Use</button>";
    row.onclick = () => {
      joinTarget = n.ssid;
      $("#joinSsid").textContent = n.ssid;
      $("#joinBox").hidden = false;
      $("#joinPass").value = "";
      $("#joinPass").focus();
    };
    $("#netList").appendChild(row);
  });
};
$("#joinBtn").onclick = async () => {
  if (!joinTarget) return;
  const mode = document.querySelector('input[name="wifiMode"]:checked').value;
  const body = new URLSearchParams({
    ssid: joinTarget,
    password: $("#joinPass").value,
    mode: mode === "ap" ? "both" : mode
  });
  const r = await fetch("/api/wifi/join", { method: "POST", body });
  const data = await r.json();
  toast(data.message || (r.ok ? "Joining…" : "Join failed"));
};

$("#preset").onchange = syncPreset;
$("#settingsForm").onsubmit = async (e) => {
  e.preventDefault();
  const f = e.target;
  const body = new URLSearchParams({
    ssid: f.ssid.value,
    password: f.password.value,
    open: f.open.checked ? "1" : "0",
    wifiMode: f.wifiMode.value,
    cs: f.cs.value, sck: f.sck.value, miso: f.miso.value, mosi: f.mosi.value
  });
  const r = await fetch("/api/settings", { method: "POST", body });
  const data = await r.json();
  toast(data.message || (r.ok ? "Saved" : "Save failed"));
};
$("#remountBtn").onclick = async () => {
  const r = await fetch("/api/remount", { method: "POST" });
  const data = await r.json();
  toast(data.message || (r.ok ? "Remounted" : "Remount failed"));
  loadStatus(); loadFiles();
};
$("#rebootBtn").onclick = async () => {
  if (!confirm("Reboot the ESP32? You will need to reconnect to the hotspot.")) return;
  await fetch("/api/reboot", { method: "POST" });
  toast("Rebooting…");
};
$("#closePreview").onclick = () => {
  const v = $("#previewBody video");
  if (v) { v.pause(); v.removeAttribute("src"); v.load(); }
  $("#preview").close();
};

(async () => {
  setView(viewMode);
  syncSort();
  await loadStatus();
  await loadSettings();
  const s = await (await fetch("/api/settings")).json();
  $("#preset").value = detectPreset(s);
  await loadFiles();
})();
</script>
</body>
</html>
)rawliteral";
