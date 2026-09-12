/*
  ESP32 SD Reader — Wi-Fi hotspot + web file browser

  1. Flash this sketch with Arduino IDE (ESP32 board support installed).
  2. Wire a SPI microSD module (FAT32) using the default DevKit pins below,
     or change them later in the Settings page.
  3. Power the ESP32. It creates a Wi-Fi hotspot:
       Name:     ESP32-SD
       Password: sdreader1
     If you later join home Wi-Fi and that network is missing after a
     power cycle, the hotspot turns on automatically so you can still
     reach the device.
  4. Connect a phone or laptop to that network and open:
       http://192.168.4.1

  Default SPI wiring (ESP32 DevKit):
       SD module        ESP32
       ---------------  -----------
       CS / SS          GPIO 5
       SCK / CLK        GPIO 18
       MISO / DO        GPIO 19
       MOSI / DI        GPIO 23
       VCC              VIN / 5V if the module has a regulator (AMS1117).
                        Use 3V3 only if the module is 3.3V-only (no regulator).
       GND              GND

  The SD card must be formatted FAT32. The web UI is stored in flash, so the
  site still loads if the card is missing — you can fix pins from Settings.

  Typical 6-pin SPI modules cannot do native SDMMC. Firmware mounts SPI only.
*/

#define DEFAULT_STORAGE_TYPE_ESP32 5  // STORAGE_SD
#define DEFAULT_FTP_SERVER_NETWORK_TYPE_ESP32 6  // NETWORK_ESP32
#define FTP_BUF_SIZE 16384
#define FTP_TIME_OUT (30 * 60)
#define HTTP_UPLOAD_BUFLEN 8192
#define SD_XFER_BUF 16384
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <Preferences.h>
#include "qrcode.h"
#include <SimpleFTPServer.h>
#include "webui.h"

static const char *DEFAULT_SSID = "ESP32-SD";
static const char *DEFAULT_PASS = "sdreader1";
static const char *FTP_USER = "sd";
static const char *FTP_PASS = "sdreader1";
static const uint8_t DEFAULT_CS = 5;
static const uint8_t DEFAULT_SCK = 18;
static const uint8_t DEFAULT_MISO = 19;
static const uint8_t DEFAULT_MOSI = 23;

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);

WebServer server(80);
DNSServer dns;
FtpServer ftpSrv;
Preferences prefs;

String apSsid;
String apPass;
String wifiMode = "ap";
String staSsid;
String staPass;
bool hotspotFallback = false;
uint8_t pinCs, pinSck, pinMiso, pinMosi;
bool sdReady = false;
bool useSdmmc = false;
String sdBus = "none";
uint32_t sdHz = 0;
SPIClass *sdSpi = nullptr;
File uploadFile;
String uploadDir = "/";
bool uploadOk = false;
String uploadError;

static uint8_t sdXferBuf[SD_XFER_BUF];

fs::FS &sdFs() {
  return useSdmmc ? static_cast<fs::FS &>(SD_MMC) : static_cast<fs::FS &>(SD);
}
bool sdExists(const String &p) { return sdFs().exists(p); }
File sdOpen(const String &p, const char *mode = FILE_READ) { return sdFs().open(p, mode); }
bool sdRemove(const String &p) { return sdFs().remove(p); }
bool sdMkdir(const String &p) { return sdFs().mkdir(p); }
bool sdRmdir(const String &p) { return sdFs().rmdir(p); }
uint64_t sdCardSize() { return useSdmmc ? SD_MMC.cardSize() : SD.cardSize(); }
uint64_t sdUsedBytes() { return useSdmmc ? SD_MMC.usedBytes() : SD.usedBytes(); }
uint8_t sdType() { return useSdmmc ? (uint8_t)SD_MMC.cardType() : (uint8_t)SD.cardType(); }

String u64str(uint64_t n) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%llu", (unsigned long long)n);
  return String(buf);
}

String jsonEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<uint8_t>(c) < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<uint8_t>(c));
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

String sanitizePath(String p) {
  p.replace('\\', '/');
  if (p.length() == 0) p = "/";
  if (!p.startsWith("/")) p = "/" + p;

  String out = "/";
  int start = 1;
  while (start <= (int)p.length()) {
    int slash = p.indexOf('/', start);
    if (slash < 0) slash = p.length();
    String part = p.substring(start, slash);
    start = slash + 1;
    if (part.length() == 0 || part == ".") continue;
    if (part == "..") {
      int prev = out.lastIndexOf('/', out.length() - 2);
      out = (prev < 0) ? "/" : out.substring(0, prev + 1);
      continue;
    }
    if (out != "/") out += "/";
    out += part;
  }
  if (out.length() == 0) out = "/";
  return out;
}

String contentType(const String &path) {
  String p = path;
  p.toLowerCase();
  if (p.endsWith(".html") || p.endsWith(".htm")) return "text/html";
  if (p.endsWith(".css")) return "text/css";
  if (p.endsWith(".js")) return "application/javascript";
  if (p.endsWith(".json")) return "application/json";
  if (p.endsWith(".png")) return "image/png";
  if (p.endsWith(".jpg") || p.endsWith(".jpeg")) return "image/jpeg";
  if (p.endsWith(".gif")) return "image/gif";
  if (p.endsWith(".webp")) return "image/webp";
  if (p.endsWith(".bmp")) return "image/bmp";
  if (p.endsWith(".svg")) return "image/svg+xml";
  if (p.endsWith(".txt") || p.endsWith(".log") || p.endsWith(".md") || p.endsWith(".ini") || p.endsWith(".csv")) return "text/plain";
  if (p.endsWith(".pdf")) return "application/pdf";
  if (p.endsWith(".mp3")) return "audio/mpeg";
  if (p.endsWith(".wav")) return "audio/wav";
  if (p.endsWith(".mp4") || p.endsWith(".m4v")) return "video/mp4";
  if (p.endsWith(".webm")) return "video/webm";
  if (p.endsWith(".mkv")) return "video/x-matroska";
  if (p.endsWith(".mov")) return "video/quicktime";
  if (p.endsWith(".avi")) return "video/x-msvideo";
  return "application/octet-stream";
}

String urlDecode(String s) {
  s.replace("+", " ");
  String out;
  out.reserve(s.length());
  for (int i = 0; i < (int)s.length(); i++) {
    if (s[i] == '%' && i + 2 < (int)s.length()) {
      char hex[3] = { (char)s[i + 1], (char)s[i + 2], 0 };
      out += (char)strtol(hex, nullptr, 16);
      i += 2;
    } else {
      out += s[i];
    }
  }
  return out;
}

String resolveSdPath(String path) {
  path = sanitizePath(path);
  if (sdExists(path)) return path;
  String once = sanitizePath(urlDecode(path));
  if (sdExists(once)) return once;
  String twice = sanitizePath(urlDecode(once));
  if (sdExists(twice)) return twice;
  return path;
}

void sendJson(int code, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void sendError(int code, const String &msg) {
  sendJson(code, "{\"error\":\"" + jsonEscape(msg) + "\"}");
}

void loadSettings() {
  prefs.begin("sdreader", false);
  apSsid = prefs.getString("ssid", DEFAULT_SSID);
  apPass = prefs.getString("pass", DEFAULT_PASS);
  wifiMode = prefs.getString("wmode", "ap");
  staSsid = prefs.getString("staSsid", "");
  staPass = prefs.getString("staPass", "");
  pinCs = prefs.getUChar("cs", DEFAULT_CS);
  pinSck = prefs.getUChar("sck", DEFAULT_SCK);
  pinMiso = prefs.getUChar("miso", DEFAULT_MISO);
  pinMosi = prefs.getUChar("mosi", DEFAULT_MOSI);
  if (apSsid.length() == 0) apSsid = DEFAULT_SSID;
  if (wifiMode != "sta" && wifiMode != "both") wifiMode = "ap";
}

bool mountSdmmc() {
  Serial.println("Trying SD_MMC 1-bit (CLK=14 CMD=15 D0=2)...");
  SD_MMC.end();
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  delay(30);
  if (!SD_MMC.setPins(14, 15, 2)) {
    Serial.println("  SD_MMC setPins failed");
    return false;
  }
  const int freqs[] = {400};
  for (int freq : freqs) {
    SD_MMC.end();
    pinMode(14, INPUT_PULLUP);
    pinMode(15, INPUT_PULLUP);
    pinMode(2, INPUT_PULLUP);
    delay(20);
    if (!SD_MMC.setPins(14, 15, 2)) continue;
    if (SD_MMC.begin("/sd", true, false, freq, 5)) {
      useSdmmc = true;
      sdBus = "SDMMC 1-bit";
      sdHz = (uint32_t)freq * 1000UL;
      sdReady = true;
      Serial.printf("SD_MMC mount OK at %d kHz type=%u size=%s\n",
                    freq, SD_MMC.cardType(), u64str(SD_MMC.cardSize()).c_str());
      return true;
    }
    Serial.printf("  SD_MMC %d kHz -> fail\n", freq);
  }
  SD_MMC.end();
  return false;
}

bool trySdBus(uint8_t spiBus, uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs, uint32_t freq, bool log = false) {
  SD.end();
  if (sdSpi) {
    sdSpi->end();
    delete sdSpi;
    sdSpi = nullptr;
  }
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  pinMode(miso, INPUT_PULLUP);
  delay(20);
  sdSpi = new SPIClass(spiBus);
  sdSpi->begin(sck, miso, mosi, cs);
  bool ok = SD.begin(cs, *sdSpi, freq, "/sd", 5);
  if (log || ok) {
    Serial.printf("  bus=%u sck=%u miso=%u mosi=%u cs=%u freq=%lu -> %s\n",
                  spiBus, sck, miso, mosi, cs, (unsigned long)freq, ok ? "OK" : "fail");
  }
  return ok;
}

bool mountSdSpi() {
  useSdmmc = false;
  sdBus = "none";
  uint8_t busUsed = 0;
  uint8_t miso = pinMiso;
  uint8_t mosi = pinMosi;
  bool found = false;
  const uint8_t buses[] = {HSPI, VSPI};
  for (uint8_t bus : buses) {
    if (trySdBus(bus, pinSck, pinMiso, pinMosi, pinCs, 400000, true)) {
      busUsed = bus;
      found = true;
      break;
    }
    if (trySdBus(bus, pinSck, pinMosi, pinMiso, pinCs, 400000, true)) {
      busUsed = bus;
      miso = pinMosi;
      mosi = pinMiso;
      found = true;
      Serial.println("SD using swapped MOSI/MISO — leave wires as they are.");
      break;
    }
  }
  if (!found) {
    Serial.printf("SD SPI FAILED (CS=%u SCK=%u MISO=%u MOSI=%u)\n",
                  pinCs, pinSck, pinMiso, pinMosi);
    return false;
  }

  const uint32_t speeds[] = {40000000, 26666666, 20000000, 16000000, 10000000, 8000000, 4000000, 1000000, 400000};
  for (uint32_t freq : speeds) {
    if (trySdBus(busUsed, pinSck, miso, mosi, pinCs, freq, freq >= 20000000)) {
      sdReady = true;
      useSdmmc = false;
      sdBus = "SPI";
      sdHz = freq;
      Serial.printf("SD SPI mount OK at %lu Hz type=%u size=%s\n",
                    (unsigned long)freq, SD.cardType(), u64str(SD.cardSize()).c_str());
      return true;
    }
  }
  Serial.println("SD SPI FAILED while raising clock");
  return false;
}

void dumpSdRoot() {
  File dir = sdOpen("/");
  if (!dir) {
    Serial.println("SD root open failed");
    return;
  }
  int n = 0;
  File f = dir.openNextFile();
  while (f && n < 12) {
    Serial.printf("  %s%s\n", f.isDirectory() ? "[dir] " : "", f.name());
    f.close();
    f = dir.openNextFile();
    n++;
  }
  dir.close();
  if (n == 0) Serial.println("  (root is empty)");
}

bool mountSd() {
  sdReady = false;
  useSdmmc = false;
  sdBus = "none";
  sdHz = 0;
  Serial.println("SD mount attempts...");
  if (!mountSdSpi()) return false;
  Serial.println("SD root:");
  dumpSdRoot();
  return true;
}

void applyHotspot() {
  const char *pass = (apPass.length() >= 8) ? apPass.c_str() : nullptr;
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(apSsid.c_str(), pass, 1, 0, 4);
  dns.start(53, "*", AP_IP);
}

bool hotspotOn() {
  wifi_mode_t m = WiFi.getMode();
  return m == WIFI_AP || m == WIFI_AP_STA;
}

bool staSsidInRange() {
  int n = WiFi.scanNetworks(false, true);
  if (n < 0) return true;
  bool found = false;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == staSsid) {
      found = true;
      break;
    }
  }
  WiFi.scanDelete();
  return found;
}

bool joinStation() {
  if (staSsid.length() == 0) return false;
  Serial.printf("Looking for Wi-Fi \"%s\"...\n", staSsid.c_str());
  if (!staSsidInRange()) {
    Serial.printf("Wi-Fi \"%s\" not found\n", staSsid.c_str());
    return false;
  }
  WiFi.setHostname("esp32-sd");
  WiFi.begin(staSsid.c_str(), staPass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(200);
    yield();
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Could not join \"%s\"\n", staSsid.c_str());
    WiFi.disconnect(false, true);
    return false;
  }
  start = millis();
  while ((uint32_t)WiFi.localIP() == 0 && millis() - start < 3000) {
    delay(50);
    yield();
  }
  return true;
}

void startMdns() {
  MDNS.end();
  if (!MDNS.begin("esp32-sd")) {
    Serial.println("mDNS failed");
    return;
  }
  MDNS.setInstanceName("ESP32 SD Reader");
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ftp", "tcp", 21);
}

void startHttp() {
  server.close();
  delay(20);
  server.begin();
}

void startWifi() {
  hotspotFallback = false;
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setHostname("esp32-sd");
  WiFi.disconnect(true, true);
  delay(50);

  bool wantSta = (wifiMode == "sta" || wifiMode == "both") && staSsid.length() > 0;

  if (wantSta && wifiMode == "both") {
    WiFi.mode(WIFI_AP_STA);
    applyHotspot();
    if (!joinStation()) {
      hotspotFallback = true;
      Serial.println("Home Wi-Fi missing — hotspot stays on");
    }
  } else if (wantSta) {
    WiFi.mode(WIFI_STA);
    if (!joinStation()) {
      hotspotFallback = true;
      WiFi.mode(WIFI_AP);
      applyHotspot();
      Serial.println("Home Wi-Fi missing — hotspot is on as fallback");
    }
  } else {
    WiFi.mode(WIFI_AP);
    applyHotspot();
  }

  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  esp_wifi_set_ps(WIFI_PS_NONE);
  startMdns();
  Serial.println();
  Serial.printf("Wi-Fi mode: %s%s\n", wifiMode.c_str(),
                hotspotFallback ? " (hotspot fallback)" : "");
  if (hotspotOn()) {
    Serial.println("Hotspot ready");
    Serial.printf("  SSID:     %s\n", apSsid.c_str());
    Serial.printf("  Password: %s\n", apPass.length() >= 8 ? apPass.c_str() : "(open network)");
    Serial.printf("  Open:     http://%s\n", WiFi.softAPIP().toString().c_str());
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Joined Wi-Fi %s\n", staSsid.c_str());
    Serial.printf("  On that network open: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.println("  (http://192.168.4.1 only works while you are on the ESP32 hotspot)");
  }
  Serial.println("  mDNS:     http://esp32-sd.local");
}

String requestHostIp() {
  IPAddress lip = server.client().localIP();
  if ((uint32_t)lip != 0) return lip.toString();
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return WiFi.softAPIP().toString();
}

void onFtpEvent(FtpOperation op, uint32_t, uint32_t) {
  if (op == FTP_CONNECT) Serial.println("FTP client connected");
  else if (op == FTP_DISCONNECT) Serial.println("FTP client disconnected");
}

void startFtp() {
  if (!sdReady) return;
  ftpSrv.end();
  delay(20);
  ftpSrv.setCallback(onFtpEvent);
  ftpSrv.begin(FTP_USER, FTP_PASS);
  IPAddress ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
  if ((uint32_t)ip == 0) ip = WiFi.softAPIP();
  ftpSrv.setLocalIp(ip);
  Serial.printf("FTP server started\n");
  if (hotspotOn()) {
    Serial.printf("  Hotspot: ftp://%s:%s@%s:21\n", FTP_USER, FTP_PASS, WiFi.softAPIP().toString().c_str());
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  Home Wi-Fi: ftp://%s:%s@%s:21\n", FTP_USER, FTP_PASS, WiFi.localIP().toString().c_str());
  }
}

String wifiQrPayload() {
  String t = "WIFI:T:";
  t += (apPass.length() >= 8) ? "WPA" : "nopass";
  t += ";S:";
  for (size_t i = 0; i < apSsid.length(); i++) {
    char c = apSsid[i];
    if (c == '\\' || c == ';' || c == ',' || c == ':') t += '\\';
    t += c;
  }
  t += ";P:";
  for (size_t i = 0; i < apPass.length(); i++) {
    char c = apPass[i];
    if (c == '\\' || c == ';' || c == ',' || c == ':') t += '\\';
    t += c;
  }
  t += ";H:false;;";
  return t;
}

static String qrBits;
static int qrSize = 0;

static void qrCollect(esp_qrcode_handle_t qrcode) {
  qrSize = esp_qrcode_get_size(qrcode);
  qrBits = "";
  qrBits.reserve(qrSize * qrSize);
  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      qrBits += esp_qrcode_get_module(qrcode, x, y) ? '1' : '0';
    }
  }
}

void handleQr() {
  String kind = server.arg("kind");
  String text;
  if (kind == "wifi") text = wifiQrPayload();
  else if (kind == "ftp") {
    text = "ftp://";
    text += FTP_USER;
    text += ":";
    text += FTP_PASS;
    text += "@";
    text += requestHostIp();
    text += ":21";
  } else {
    text = "http://";
    text += requestHostIp();
  }

  qrBits = "";
  qrSize = 0;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = qrCollect;
  cfg.max_qrcode_version = 10;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;
  if (esp_qrcode_generate(&cfg, text.c_str()) != ESP_OK || qrSize == 0) {
    sendError(500, "Could not build QR code");
    return;
  }
  sendJson(200, "{\"size\":" + String(qrSize) + ",\"bits\":\"" + qrBits + "\"}");
}

void handleIndex() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  uint64_t total = sdReady ? sdCardSize() : 0;
  uint64_t used = sdReady ? sdUsedBytes() : 0;
  const char *type = "none";
  if (sdReady) {
    switch (sdType()) {
      case CARD_MMC: type = "MMC"; break;
      case CARD_SD: type = "SD"; break;
      case CARD_SDHC: type = "SDHC"; break;
      default: type = "unknown"; break;
    }
  }
  String apIp = WiFi.softAPIP().toString();
  String staIp = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  String body = "{";
  body += "\"sd\":" + String(sdReady ? "true" : "false");
  body += ",\"cardType\":\"" + String(type) + "\"";
  body += ",\"total\":" + u64str(total);
  body += ",\"used\":" + u64str(used);
  body += ",\"clients\":" + String(WiFi.softAPgetStationNum());
  body += ",\"uptime\":" + String(millis());
  body += ",\"heap\":" + String(ESP.getFreeHeap());
  body += ",\"ip\":\"" + apIp + "\"";
  body += ",\"staIp\":\"" + staIp + "\"";
  body += ",\"wifiMode\":\"" + jsonEscape(wifiMode) + "\"";
  body += ",\"staSsid\":\"" + jsonEscape(staSsid) + "\"";
  body += ",\"staConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  body += ",\"hotspot\":" + String(hotspotOn() ? "true" : "false");
  body += ",\"hotspotFallback\":" + String(hotspotFallback ? "true" : "false");
  body += ",\"ssid\":\"" + jsonEscape(apSsid) + "\"";
  body += ",\"wifiPass\":\"" + jsonEscape(apPass) + "\"";
  body += ",\"web\":\"http://" + apIp + "\"";
  if (staIp.length()) body += ",\"webLan\":\"http://" + staIp + "\"";
  else body += ",\"webLan\":\"\"";
  body += ",\"ftp\":\"ftp://" + requestHostIp() + ":21\"";
  if (staIp.length()) body += ",\"ftpLan\":\"ftp://" + String(FTP_USER) + ":" + String(FTP_PASS) + "@" + staIp + ":21\"";
  else body += ",\"ftpLan\":\"\"";
  body += ",\"vlc\":\"http://" + requestHostIp() + "/media/\"";
  body += ",\"ftpUser\":\"" + String(FTP_USER) + "\"";
  body += ",\"ftpPass\":\"" + String(FTP_PASS) + "\"";
  body += ",\"ftpPort\":21";
  body += ",\"sdHz\":" + String((unsigned long)sdHz);
  body += ",\"sdBus\":\"" + jsonEscape(sdBus) + "\"";
  body += "}";
  sendJson(200, body);
}

void appendDirEntries(File &dir, bool wantDir, String &body, bool &first) {
  File f = dir.openNextFile();
  while (f) {
    if (f.isDirectory() == wantDir) {
      if (!first) body += ",";
      first = false;
      String name = f.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      body += "{\"name\":\"" + jsonEscape(name) + "\"";
      body += ",\"dir\":" + String(wantDir ? "true" : "false");
      body += ",\"size\":" + u64str(f.size()) + "}";
    }
    f.close();
    f = dir.openNextFile();
  }
}

void handleList() {
  if (!sdReady) {
    sendError(503, "SD card is not mounted");
    return;
  }
  String path = sanitizePath(server.arg("path"));
  File dir = sdOpen(path);
  if (!dir) {
    sendError(404, "Folder not found");
    return;
  }
  if (!dir.isDirectory()) {
    dir.close();
    sendError(400, "Not a folder");
    return;
  }

  String body = "{\"path\":\"" + jsonEscape(path) + "\",\"items\":[";
  bool first = true;
  appendDirEntries(dir, true, body, first);
  dir.close();
  dir = sdOpen(path);
  if (dir) {
    appendDirEntries(dir, false, body, first);
    dir.close();
  }
  body += "]}";
  sendJson(200, body);
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Range");
  server.sendHeader("Access-Control-Allow-Methods", "GET,HEAD,OPTIONS");
  server.sendHeader("Access-Control-Expose-Headers", "Content-Length,Content-Range,Accept-Ranges");
}

void serveSdFile(String path, bool download) {
  if (!sdReady) {
    sendError(503, "SD card is not mounted");
    return;
  }
  path = resolveSdPath(path);
  if (path == "/") {
    sendError(400, "Pick a file");
    return;
  }
  if (!sdExists(path)) {
    sendError(404, "File not found");
    return;
  }
  File f = sdOpen(path, FILE_READ);
  if (!f || f.isDirectory()) {
    if (f) f.close();
    sendError(400, "Cannot read that path");
    return;
  }
  f.setBufferSize(SD_XFER_BUF);

  const size_t fileSize = f.size();
  size_t start = 0;
  size_t end = fileSize ? fileSize - 1 : 0;
  bool ranged = false;
  if (server.hasHeader("Range")) {
    String range = server.header("Range");
    if (range.startsWith("bytes=")) {
      range = range.substring(6);
      int dash = range.indexOf('-');
      String a = range.substring(0, dash);
      String b = range.substring(dash + 1);
      if (a.length()) start = a.toInt();
      if (b.length()) end = b.toInt();
      ranged = true;
    }
  }
  sendCors();
  if (fileSize == 0) {
    server.send(200, contentType(path), "");
    f.close();
    return;
  }
  if (start >= fileSize || end < start) {
    server.sendHeader("Content-Range", "bytes */" + String(fileSize));
    server.send(416, "text/plain", "Range Not Satisfiable");
    f.close();
    return;
  }
  if (end >= fileSize) end = fileSize - 1;
  size_t len = end - start + 1;
  // One HTTP client at a time. A 20 MB HEVC GET blocks the next videos.
  // Cap media slices so players re-request Range and swipe/abort can recover.
  const size_t slice = 256 * 1024;
  if (!download && len > slice) {
    end = start + slice - 1;
    len = slice;
    ranged = true;
  }

  String name = path.substring(path.lastIndexOf('/') + 1);
  if (download) {
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  } else {
    server.sendHeader("Content-Disposition", "inline; filename=\"" + name + "\"");
  }
  server.sendHeader("Accept-Ranges", "bytes");
  server.sendHeader("Cache-Control", "public, max-age=3600");
  if (ranged) {
    server.sendHeader("Content-Range", "bytes " + String(start) + "-" + String(end) + "/" + String((unsigned long)fileSize));
  }
  server.setContentLength(len);
  server.send(ranged ? 206 : 200, contentType(path), "");
  if (server.method() == HTTP_HEAD) {
    f.close();
    return;
  }

  f.seek(start);
  WiFiClient client = server.client();
  client.setNoDelay(true);
  client.setTimeout(30000);
  size_t remain = len;
  uint16_t stall = 0;
  uint32_t lastYield = millis();
  while (remain && client.connected()) {
    size_t chunk = remain > SD_XFER_BUF ? SD_XFER_BUF : remain;
    int n = f.read(sdXferBuf, chunk);
    if (n <= 0) break;
    size_t sent = 0;
    while (sent < (size_t)n && client.connected()) {
      int w = client.write(sdXferBuf + sent, n - sent);
      if (w > 0) {
        sent += w;
        stall = 0;
      } else {
        yield();
        if (++stall > 80) break;
      }
    }
    if (sent < (size_t)n) break;
    remain -= sent;
    if ((uint32_t)(millis() - lastYield) >= 20) {
      yield();
      lastYield = millis();
    }
  }
  f.close();
}

void handleFile() {
  serveSdFile(server.arg("path"), server.arg("download") == "1");
}

void handlePlay() {
  serveSdFile(server.arg("path"), false);
}

void handleDelete() {
  if (!sdReady) {
    sendError(503, "SD card is not mounted");
    return;
  }
  String path = sanitizePath(server.arg("path"));
  if (path == "/") {
    sendError(400, "Cannot delete the root folder");
    return;
  }
  if (!sdExists(path)) {
    sendError(404, "Not found");
    return;
  }
  File f = sdOpen(path);
  bool isDir = f && f.isDirectory();
  if (f) f.close();
  bool ok = isDir ? sdRmdir(path) : sdRemove(path);
  if (!ok) {
    sendError(500, isDir ? "Folder must be empty before it can be deleted" : "Delete failed");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void handleMkdir() {
  if (!sdReady) {
    sendError(503, "SD card is not mounted");
    return;
  }
  String path = sanitizePath(server.arg("path"));
  if (path == "/") {
    sendError(400, "Invalid folder name");
    return;
  }
  if (sdExists(path)) {
    sendError(409, "Already exists");
    return;
  }
  if (!sdMkdir(path)) {
    sendError(500, "Could not create folder");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void handleUpload() {
  HTTPUpload &up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    uploadOk = false;
    uploadError = "";
    if (!sdReady) {
      uploadError = "SD card is not mounted";
      return;
    }
    uploadDir = server.hasArg("path") ? sanitizePath(server.arg("path")) : "/";
    String name = up.filename;
    name.replace('\\', '/');
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    name.replace(":", "_");
    if (name.length() == 0) name = "upload.bin";
    String dest = (uploadDir == "/") ? ("/" + name) : (uploadDir + "/" + name);
    if (sdExists(dest)) sdRemove(dest);
    uploadFile = sdOpen(dest, FILE_WRITE);
    if (!uploadFile) {
      uploadError = "Could not create " + dest;
      return;
    }
    uploadFile.setBufferSize(SD_XFER_BUF);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      size_t written = uploadFile.write(up.buf, up.currentSize);
      if (written != up.currentSize) {
        uploadError = "SD write failed";
      }
      yield();
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
    }
    uploadOk = (uploadError.length() == 0);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
    }
    uploadError = "Upload aborted";
  }
}

void handleUploadDone() {
  if (!sdReady) {
    sendError(503, "SD card is not mounted");
    return;
  }
  if (!uploadOk) {
    sendError(500, uploadError.length() ? uploadError : "Upload failed");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void handleGetSettings() {
  String body = "{";
  body += "\"ssid\":\"" + jsonEscape(apSsid) + "\"";
  body += ",\"hasPassword\":" + String(apPass.length() >= 8 ? "true" : "false");
  body += ",\"wifiMode\":\"" + jsonEscape(wifiMode) + "\"";
  body += ",\"staSsid\":\"" + jsonEscape(staSsid) + "\"";
  body += ",\"staConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  body += ",\"hotspot\":" + String(hotspotOn() ? "true" : "false");
  body += ",\"hotspotFallback\":" + String(hotspotFallback ? "true" : "false");
  body += ",\"staIp\":\"" + ((WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "") + "\"";
  body += ",\"cs\":" + String(pinCs);
  body += ",\"sck\":" + String(pinSck);
  body += ",\"miso\":" + String(pinMiso);
  body += ",\"mosi\":" + String(pinMosi);
  body += "}";
  sendJson(200, body);
}

void handleScanWifi() {
  wifi_mode_t prev = WiFi.getMode();
  if (prev == WIFI_AP) WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks(false, true);
  String body = "[";
  for (int i = 0; i < n; i++) {
    if (i) body += ",";
    bool sec = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    body += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\"";
    body += ",\"rssi\":" + String(WiFi.RSSI(i));
    body += ",\"secure\":" + String(sec ? "true" : "false") + "}";
  }
  body += "]";
  WiFi.scanDelete();
  if (wifiMode == "ap" || hotspotFallback) WiFi.mode(WIFI_AP);
  sendJson(200, body);
}

void handleJoinWifi() {
  String ssid = server.arg("ssid");
  ssid.trim();
  String pass = server.arg("password");
  String mode = server.arg("mode");
  if (ssid.length() == 0) {
    sendError(400, "Pick a network");
    return;
  }
  if (mode != "sta" && mode != "both") mode = "both";
  staSsid = ssid;
  staPass = pass;
  wifiMode = mode;
  prefs.putString("staSsid", staSsid);
  prefs.putString("staPass", staPass);
  prefs.putString("wmode", wifiMode);
  sendJson(200, "{\"ok\":true,\"message\":\"Connecting. The page may disconnect if the hotspot changes.\"}");
  delay(200);
  startWifi();
  startHttp();
  startFtp();
}

void handleSaveSettings() {
  String ssid = server.arg("ssid");
  ssid.trim();
  if (ssid.length() == 0 || ssid.length() > 31) {
    sendError(400, "SSID must be 1–31 characters");
    return;
  }
  String pass = server.arg("password");
  bool openNet = server.arg("open") == "1";
  if (!openNet && pass.length() > 0 && pass.length() < 8) {
    sendError(400, "Password must be at least 8 characters");
    return;
  }
  uint8_t cs = server.arg("cs").toInt();
  uint8_t sck = server.arg("sck").toInt();
  uint8_t miso = server.arg("miso").toInt();
  uint8_t mosi = server.arg("mosi").toInt();
  String mode = server.arg("wifiMode");
  if (mode == "sta" || mode == "both" || mode == "ap") wifiMode = mode;

  apSsid = ssid;
  if (openNet) apPass = "";
  else if (pass.length() >= 8) apPass = pass;

  prefs.putString("ssid", apSsid);
  prefs.putString("pass", apPass);
  prefs.putString("wmode", wifiMode);
  prefs.putUChar("cs", cs);
  prefs.putUChar("sck", sck);
  prefs.putUChar("miso", miso);
  prefs.putUChar("mosi", mosi);

  pinCs = cs;
  pinSck = sck;
  pinMiso = miso;
  pinMosi = mosi;
  startWifi();
  startHttp();

  sendJson(200, "{\"ok\":true,\"message\":\"Saved. Reconnect if the Wi-Fi name or mode changed.\"}");
}

void handleRemount() {
  bool ok = mountSd();
  if (ok) startFtp();
  sendJson(ok ? 200 : 500, ok ? "{\"ok\":true,\"message\":\"SD card mounted\"}"
                             : "{\"ok\":false,\"message\":\"SD card did not mount\"}");
}

void handleReboot() {
  sendJson(200, "{\"ok\":true,\"message\":\"Rebooting\"}");
  delay(250);
  ESP.restart();
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    sendCors();
    server.send(204);
    return;
  }
  String uri = server.uri();
  if (uri.startsWith("/v.") && server.hasArg("path")) {
    serveSdFile(server.arg("path"), false);
    return;
  }
  if (uri.startsWith("/media/") || uri == "/media") {
    String path = (uri == "/media") ? server.arg("path") : uri.substring(6);
    serveSdFile(path, false);
    return;
  }
  if (uri.startsWith("/api/")) {
    sendError(404, "Not found");
    return;
  }
  handleIndex();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32 SD Reader");

  loadSettings();
  mountSd();
  startWifi();
  startFtp();

  server.on("/", HTTP_GET, handleIndex);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/list", HTTP_GET, handleList);
  server.on("/api/file", HTTP_GET, handleFile);
  server.on("/api/file", HTTP_HEAD, handleFile);
  const char *playExt[] = {"/v.mp4", "/v.m4v", "/v.webm", "/v.mkv", "/v.mov", "/v.avi"};
  for (const char *p : playExt) {
    server.on(p, HTTP_GET, handlePlay);
    server.on(p, HTTP_HEAD, handlePlay);
  }
  server.on("/api/delete", HTTP_POST, handleDelete);
  server.on("/api/mkdir", HTTP_POST, handleMkdir);
  server.on("/api/upload", HTTP_POST, handleUploadDone, handleUpload);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/remount", HTTP_POST, handleRemount);
  server.on("/api/qr", HTTP_GET, handleQr);
  server.on("/api/scan", HTTP_GET, handleScanWifi);
  server.on("/api/wifi/join", HTTP_POST, handleJoinWifi);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/generate_204", HTTP_GET, []() { server.send(204); });
  server.on("/gen_204", HTTP_GET, []() { server.send(204); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); });
  server.on("/ncsi.txt", HTTP_GET, []() { server.send(200, "text/plain", "Microsoft NCSI"); });
  server.on("/connecttest.txt", HTTP_GET, []() { server.send(200, "text/plain", "OK"); });
  server.onNotFound(handleNotFound);
  const char *hdrs[] = {"Range"};
  server.collectHeaders(hdrs, 1);
  server.begin();
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
  if (sdReady) ftpSrv.handleFTP();
}
