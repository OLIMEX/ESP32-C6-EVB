/*
  OLIMEX ESP32-C6-EVB Wi-Fi Web Server Demo
  SPDX-License-Identifier: GPL-3.0-or-later

  Add local Wi-Fi credentials below if station mode is wanted. If WIFI_SSID is
  empty, or the network cannot be reached, the board starts the open access
  point "OLIMEX-C6-EVB". USB serial always prints the web address.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <SPI.h>

// ---------- User configuration ----------
// Leave both strings empty to use the fallback OLIMEX-C6-EVB access point.
// For station mode, enter a local 2.4 GHz Wi-Fi SSID and password here.
const char *WIFI_SSID = "";
const char *WIFI_PASSWORD = "";
const char *AP_SSID = "OLIMEX-C6-EVB";
const char *MDNS_NAME = "esp32-c6-evb";
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

// ---------- OLIMEX ESP32-C6-EVB Rev. A pins ----------
const uint8_t RELAY_PINS[4] = {10, 11, 22, 23}; // active HIGH
const uint8_t USER_LED_PIN = 8;                  // active LOW
const uint8_t USER_BUTTON_PIN = 9;               // active LOW
const uint8_t OPTO_PINS[4] = {1, 2, 3, 15};     // active LOW

// UEXT standard connector assignment (pins 1/2 are 3.3 V/GND).
const uint8_t UEXT_UART_TX_PIN = 5;  // UEXT pin 3
const uint8_t UEXT_UART_RX_PIN = 4;  // UEXT pin 4
const uint8_t UEXT_I2C_SCL_PIN = 7;  // UEXT pin 5
const uint8_t UEXT_I2C_SDA_PIN = 6;  // UEXT pin 6
const uint8_t UEXT_SPI_MISO_PIN = 20; // UEXT pin 7
const uint8_t UEXT_SPI_MOSI_PIN = 18; // UEXT pin 8
const uint8_t UEXT_SPI_SCK_PIN = 19;  // UEXT pin 9
const uint8_t UEXT_SPI_CS_PIN = 21;   // UEXT pin 10
const uint32_t UEXT_UART_BAUD = 115200;
const uint32_t UEXT_I2C_CLOCK = 100000;
const uint32_t UEXT_SPI_CLOCK = 1000000;

const unsigned long BUTTON_DEBOUNCE_MS = 20;
const unsigned long SERIAL_ADDRESS_REMINDER_MS = 30000;
WebServer server(80);
HardwareSerial uextUart(1);
SPIClass uextSpi(FSPI);
String uextUartRx;

bool relayOn[4] = {false, false, false, false};
bool userLedOn = false;
bool buttonPressed = false;
bool lastRawButtonPressed = false;
unsigned long lastButtonChangeMs = 0;
uint8_t nextButtonStep = 0;
bool accessPointMode = false;
unsigned long lastSerialAddressReminderMs = 0;

// Complete responsive UI stored in flash; no filesystem is required.
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OLIMEX ESP32-C6-EVB</title>
<style>
:root{color-scheme:dark;--bg:#07111f;--panel:#101e31;--line:#263b55;--text:#edf6ff;--muted:#8ca2b8;--on:#35e58a;--off:#526579;--accent:#32a9ff;--danger:#ff6275}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at top,#153454 0,var(--bg) 48%);font:16px system-ui,sans-serif;color:var(--text);min-height:100vh}.wrap{max-width:960px;margin:auto;padding:28px 18px 50px}header{display:flex;justify-content:space-between;align-items:end;gap:20px;margin-bottom:22px}h1{margin:0;font-size:clamp(1.7rem,5vw,2.7rem)}h2{font-size:1.05rem;margin:0 0 14px}.sub,.small{color:var(--muted)}.online{display:flex;align-items:center;gap:8px}.dot{display:inline-block;flex:0 0 12px;width:12px;height:12px;border-radius:50%;background:var(--off);box-shadow:0 0 0 3px #ffffff12;vertical-align:middle}.dot.on{background:var(--on);box-shadow:0 0 12px var(--on)}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}.panel{background:#101e31e8;border:1px solid var(--line);border-radius:16px;padding:18px;box-shadow:0 12px 30px #0004}.wide{grid-column:1/-1}.relays,.inputs{display:grid;grid-template-columns:repeat(4,1fr);gap:11px}.card{background:#0a1626;border:1px solid var(--line);border-radius:12px;padding:14px}.card-head,.row{display:flex;align-items:center;justify-content:space-between;gap:8px}.row{font-size:.86rem;color:var(--muted);margin-top:11px}.state{font-weight:800;color:var(--muted)}.state.on{color:var(--on)}button{width:100%;border:0;border-radius:9px;padding:11px 9px;margin-top:13px;background:#1c87d3;color:white;font-weight:750;cursor:pointer;touch-action:manipulation}button:hover{filter:brightness(1.12)}button:active{transform:translateY(1px)}.controls{display:flex;gap:10px}.controls button{margin:0}.relay-actions,.led-control{display:grid;grid-template-columns:1fr 1fr;gap:7px}.offbtn{background:#34495e}.danger{background:#bd3f51}.sequence{padding:11px 13px;background:#081422;border-radius:9px;color:var(--muted);line-height:1.55}.error{display:none;color:#ff91a0;margin-top:10px}.uext-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}.tool{background:#0a1626;border:1px solid var(--line);border-radius:12px;padding:14px}.pinout{font:12px ui-monospace,monospace;color:var(--muted);line-height:1.55}input,textarea{width:100%;background:#06101d;border:1px solid var(--line);border-radius:8px;color:var(--text);padding:10px;margin-top:8px}textarea{height:105px;resize:vertical;font-family:ui-monospace,monospace}.result{min-height:42px;white-space:pre-wrap;word-break:break-word}.check{display:flex;align-items:center;gap:7px;margin-top:8px;color:var(--muted);font-size:.85rem}.check input{width:auto;margin:0}@media(max-width:700px){header{display:block}.online{margin-top:12px}.grid{grid-template-columns:1fr}.relays,.inputs{grid-template-columns:repeat(2,1fr)}.uext-grid{grid-template-columns:1fr}}
</style></head><body><main class="wrap">
<header><div><h1>ESP32-C6-EVB</h1><div class="sub">Wi-Fi relay &amp; input control</div></div><div class="online"><span id="netDot" class="dot"></span><span id="net">Connecting…</span></div></header>
<div class="grid">
<section class="panel wide"><h2>Relays</h2><div class="relays" id="relays"></div><div class="controls" style="margin-top:12px"><button onclick="allRelays('on')">All ON</button><button class="danger" onclick="allRelays('off')">All OFF</button></div></section>
<section class="panel"><h2>Optocoupler inputs</h2><div class="inputs" id="optos"></div><p class="small">ACTIVE means voltage is present at the corresponding isolated input.</p></section>
<section class="panel"><h2>User LED</h2><div class="card-head"><span>GPIO 8</span><span><i id="userDot" class="dot"></i> <b id="userState" class="state">OFF</b></span></div><div class="led-control"><button onclick="setLed('toggle')">Toggle</button><button class="offbtn" onclick="setLed('off')">Off</button></div></section>
<section class="panel wide"><h2>Button 1</h2><div class="card-head"><span>Physical button</span><b id="buttonState" class="state">RELEASED</b></div><p class="sequence">Each press selects: Relay 1 → Relay 2 → Relay 3 → Relay 4 → all ON → all OFF → repeat.<br>Next press: <b id="nextStep">Relay 1</b></p><div id="err" class="error">Communication lost. Retrying…</div></section>
<section class="panel wide"><h2>UEXT diagnostics</h2><p class="pinout">1: +3.3V · 2: GND · 3: UART TX/GPIO5 · 4: UART RX/GPIO4 · 5: I²C SCL/GPIO7 · 6: I²C SDA/GPIO6 · 7: SPI MISO/GPIO20 · 8: SPI MOSI/GPIO18 · 9: SPI CLK/GPIO19 · 10: SPI CS/GPIO21</p><div class="uext-grid">
<div class="tool"><b>UART · 115200 8N1</b><textarea id="uartLog" readonly placeholder="Received data"></textarea><input id="uartTx" placeholder="Text to transmit"><label class="check"><input id="uartNl" type="checkbox" checked>Append newline</label><button onclick="uartSend()">Send on pin 3</button></div>
<div class="tool"><b>I²C · 100 kHz</b><p class="small">Scans valid 7-bit addresses on pins 5/6.</p><div id="i2cResult" class="result small">Not scanned</div><button onclick="i2cScan()">Scan bus</button></div>
<div class="tool"><b>SPI · 1 MHz, mode 0</b><p class="small">CS pin 10 is asserted for one transaction.</p><input id="spiTx" value="9F" placeholder="TX hex, e.g. 9F 00 00 00"><div id="spiResult" class="result small">RX: —</div><button onclick="spiTransfer()">Transfer</button></div>
</div></section>
</div></main>
<script>
const $=id=>document.getElementById(id);
let lastRelayKey='',lastOptoKey='';
function statusText(el,on,a='ON',b='OFF'){el.textContent=on?a:b;el.classList.toggle('on',on)}
function render(s){
 $('netDot').classList.add('on');$('net').textContent=`${s.mode} · ${s.ip}${s.rssi===null?'':` · ${s.rssi} dBm`}`;
 const relayKey=s.relays.join('')+'|'+s.relayLeds.join('');
 if(relayKey!==lastRelayKey){$('relays').innerHTML=s.relays.map((on,i)=>`<div class="card"><div class="card-head"><b>Relay ${i+1}</b><span class="state ${on?'on':''}">${on?'ON':'OFF'}</span></div><div class="row"><span>Relay LED</span><span><i class="dot ${s.relayLeds[i]?'on':''}"></i> ${s.relayLeds[i]?'ON':'OFF'}</span></div><div class="relay-actions"><button onclick="relay(${i+1},'on')">ON</button><button class="offbtn" onclick="relay(${i+1},'off')">OFF</button></div></div>`).join('');lastRelayKey=relayKey}
 const optoKey=s.optos.join('');
 if(optoKey!==lastOptoKey){$('optos').innerHTML=s.optos.map((on,i)=>`<div class="card"><div class="card-head"><b>IN${i+1}</b><i class="dot ${on?'on':''}"></i></div><div class="state ${on?'on':''}" style="margin-top:12px">${on?'ACTIVE':'IDLE'}</div></div>`).join('');lastOptoKey=optoKey}
 $('userDot').classList.toggle('on',s.userLed);statusText($('userState'),s.userLed);statusText($('buttonState'),s.button,'PRESSED','RELEASED');
 $('nextStep').textContent=s.nextButtonStep;$('err').style.display='none';
}
async function call(url){try{const r=await fetch(url,{method:'POST'});if(!r.ok)throw Error();render(await r.json())}catch(e){$('err').style.display='block'}}
const relay=(id,state)=>call(`/api/relay?id=${id}&state=${state}`);
const allRelays=state=>call(`/api/relays?state=${state}`);
const setLed=state=>call(`/api/led?state=${state}`);
async function uartSend(){const d=$('uartTx').value,n=$('uartNl').checked?'1':'0';const r=await fetch(`/api/uext/uart?newline=${n}&data=${encodeURIComponent(d)}`,{method:'POST'});if(r.ok)$('uartTx').value=''}
let uartBusy=false,pollBusy=false;
async function uartRead(){if(uartBusy)return;uartBusy=true;try{const r=await fetch('/api/uext/uart',{cache:'no-store'}),t=await r.text();if(t){$('uartLog').value+=t;$('uartLog').scrollTop=$('uartLog').scrollHeight}}catch(e){}finally{uartBusy=false}}
async function i2cScan(){const e=$('i2cResult');e.textContent='Scanning…';try{const r=await fetch('/api/uext/i2c-scan',{method:'POST'}),j=await r.json();e.textContent=j.addresses.length?'Found: '+j.addresses.join(', '):'No devices found'}catch(x){e.textContent='Scan failed'}}
async function spiTransfer(){const e=$('spiResult');try{const r=await fetch('/api/uext/spi?tx='+encodeURIComponent($('spiTx').value),{method:'POST'}),j=await r.json();e.textContent='RX: '+j.rx}catch(x){e.textContent='Invalid transfer'}}
async function poll(){if(pollBusy)return;pollBusy=true;try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error();render(await r.json())}catch(e){$('netDot').classList.remove('on');$('net').textContent='Offline';$('err').style.display='block'}finally{pollBusy=false}}
poll();setInterval(poll,200);setInterval(uartRead,500);
</script></body></html>
)HTML";

void setRelay(uint8_t index, bool on) {
  // Track the commanded state because the relay contacts have no feedback input.
  if (index >= 4) return;
  relayOn[index] = on;
  digitalWrite(RELAY_PINS[index], on ? HIGH : LOW);
}

void setAllRelays(bool on) {
  for (uint8_t i = 0; i < 4; ++i) setRelay(i, on);
}

void setUserLed(bool on) {
  userLedOn = on;
  digitalWrite(USER_LED_PIN, on ? LOW : HIGH);
}

const char *buttonStepName(uint8_t step) {
  static const char *NAMES[6] = {"Relay 1", "Relay 2", "Relay 3", "Relay 4", "All ON", "All OFF"};
  return NAMES[step % 6];
}

void runButtonStep() {
  // Six stages: R1, R2, R3, R4, all on, then all off.
  setAllRelays(false);
  if (nextButtonStep < 4) setRelay(nextButtonStep, true);
  else if (nextButtonStep == 4) setAllRelays(true);

  Serial.print("Button sequence: ");
  Serial.println(buttonStepName(nextButtonStep));
  nextButtonStep = (nextButtonStep + 1) % 6;
}

String makeStatusJson() {
  // Compact response polled by the browser for live hardware state.
  String json;
  json.reserve(300);
  json = F("{\"relays\":[");
  for (uint8_t i = 0; i < 4; ++i) { if (i) json += ','; json += relayOn[i] ? F("true") : F("false"); }
  json += F("],\"relayLeds\":[");
  for (uint8_t i = 0; i < 4; ++i) { if (i) json += ','; json += relayOn[i] ? F("true") : F("false"); }
  json += F("],\"optos\":[");
  for (uint8_t i = 0; i < 4; ++i) { if (i) json += ','; json += digitalRead(OPTO_PINS[i]) == LOW ? F("true") : F("false"); }
  json += F("],\"userLed\":"); json += userLedOn ? F("true") : F("false");
  json += F(",\"button\":"); json += buttonPressed ? F("true") : F("false");
  json += F(",\"nextButtonStep\":\""); json += buttonStepName(nextButtonStep);
  json += F("\",\"mode\":\""); json += accessPointMode ? F("Access point") : F("Wi-Fi");
  json += F("\",\"ip\":\""); json += accessPointMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  json += F("\",\"rssi\":");
  if (accessPointMode) json += F("null"); else json += WiFi.RSSI();
  json += '}';
  return json;
}

void sendStatus() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", makeStatusJson());
}

void printNetworkInfo() {
  IPAddress ip = accessPointMode ? WiFi.softAPIP() : WiFi.localIP();
  Serial.println();
  Serial.println(F("========== WEB PAGE =========="));
  Serial.print(F("Mode: "));
  Serial.println(accessPointMode ? F("Wi-Fi access point") : F("Connected to Wi-Fi router"));
  Serial.print(F("Network: "));
  Serial.println(accessPointMode ? AP_SSID : WIFI_SSID);
  Serial.print(F("Open in browser: http://"));
  Serial.print(ip);
  Serial.println('/');
  Serial.print(F("mDNS address:    http://"));
  Serial.print(MDNS_NAME);
  Serial.println(F(".local/"));
  if (accessPointMode) Serial.println(F("First connect your phone/PC to Wi-Fi: OLIMEX-C6-EVB"));
  Serial.println(F("Serial speed: 115200 baud"));
  Serial.println(F("=============================="));
}

void printHardwareStatus() {
  Serial.print(F("Relays: "));
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.print(relayOn[i] ? F("ON") : F("OFF"));
    if (i < 3) Serial.print(F(", "));
  }
  Serial.print(F(" | User LED: "));
  Serial.println(userLedOn ? F("ON") : F("OFF"));
  Serial.print(F("Optocouplers: "));
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.print(digitalRead(OPTO_PINS[i]) == LOW ? F("ACTIVE") : F("IDLE"));
    if (i < 3) Serial.print(F(", "));
  }
  Serial.println();
}

bool wantedState(const String &value, bool current, bool &valid) {
  valid = true;
  if (value == "on") return true;
  if (value == "off") return false;
  if (value == "toggle") return !current;
  valid = false;
  return current;
}

int8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool parseHexBytes(const String &text, uint8_t *bytes, size_t &count) {
  // Accept byte pairs separated by spaces, commas, colons or dashes.
  count = 0;
  size_t i = 0;
  while (i < text.length()) {
    while (i < text.length() && (text[i] == ' ' || text[i] == ',' || text[i] == ':' || text[i] == '-')) ++i;
    if (i >= text.length()) break;
    if (i + 1 < text.length() && text[i] == '0' && (text[i + 1] == 'x' || text[i + 1] == 'X')) i += 2;
    if (i + 1 >= text.length() || count >= 32) return false;
    int8_t high = hexNibble(text[i++]);
    int8_t low = hexNibble(text[i++]);
    if (high < 0 || low < 0) return false;
    bytes[count++] = (uint8_t)((high << 4) | low);
  }
  return count > 0;
}

void sendUartRx() {
  String data = uextUartRx;
  uextUartRx = "";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain; charset=utf-8", data);
}

void scanI2c() {
  // Scan the standard 7-bit application range; skip reserved addresses.
  String json = F("{\"addresses\":[");
  bool first = true;
  char addressText[7];
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      if (!first) json += ',';
      snprintf(addressText, sizeof(addressText), "\"0x%02X\"", address);
      json += addressText;
      first = false;
    }
  }
  json += F("]}");
  server.send(200, "application/json", json);
}

void transferSpi() {
  // Keep chip select active for the complete full-duplex transaction.
  uint8_t data[32];
  size_t count = 0;
  if (!parseHexBytes(server.arg("tx"), data, count)) {
    server.send(400, "text/plain", "Use 1-32 complete hexadecimal bytes");
    return;
  }

  uextSpi.beginTransaction(SPISettings(UEXT_SPI_CLOCK, MSBFIRST, SPI_MODE0));
  digitalWrite(UEXT_SPI_CS_PIN, LOW);
  for (size_t i = 0; i < count; ++i) data[i] = uextSpi.transfer(data[i]);
  digitalWrite(UEXT_SPI_CS_PIN, HIGH);
  uextSpi.endTransaction();

  String json = F("{\"rx\":\"");
  char byteText[4];
  for (size_t i = 0; i < count; ++i) {
    if (i) json += ' ';
    snprintf(byteText, sizeof(byteText), "%02X", data[i]);
    json += byteText;
  }
  json += F("\"}");
  server.send(200, "application/json", json);
}

void configureWebServer() {
  // WebServer is synchronous. Remove its idle delay for fast controls; loop()
  // still yields one millisecond so the Wi-Fi task can run.
  server.enableDelay(false);
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/relay", HTTP_POST, []() {
    int id = server.arg("id").toInt();
    bool valid = false;
    if (id < 1 || id > 4) { server.send(400, "text/plain", "Invalid relay id"); return; }
    bool on = wantedState(server.arg("state"), relayOn[id - 1], valid);
    if (!valid) { server.send(400, "text/plain", "Invalid state"); return; }
    setRelay(id - 1, on);
    Serial.print(F("[WEB] Relay ")); Serial.print(id); Serial.println(on ? F(" ON") : F(" OFF"));
    sendStatus();
  });
  server.on("/api/relays", HTTP_POST, []() {
    String state = server.arg("state");
    if (state != "on" && state != "off") { server.send(400, "text/plain", "Invalid state"); return; }
    setAllRelays(state == "on");
    Serial.println(state == "on" ? F("[WEB] All relays ON") : F("[WEB] All relays OFF"));
    sendStatus();
  });
  server.on("/api/led", HTTP_POST, []() {
    bool valid = false;
    bool on = wantedState(server.arg("state"), userLedOn, valid);
    if (!valid) { server.send(400, "text/plain", "Invalid state"); return; }
    setUserLed(on);
    Serial.println(on ? F("[WEB] User LED ON") : F("[WEB] User LED OFF"));
    sendStatus();
  });
  server.on("/api/uext/uart", HTTP_GET, sendUartRx);
  server.on("/api/uext/uart", HTTP_POST, []() {
    String data = server.arg("data");
    if (data.length() > 256) { server.send(413, "text/plain", "UART message too long"); return; }
    uextUart.print(data);
    if (server.arg("newline") == "1") uextUart.println();
    Serial.print(F("[UEXT UART] Sent ")); Serial.print(data.length()); Serial.println(F(" character(s)"));
    server.send(204);
  });
  server.on("/api/uext/i2c-scan", HTTP_POST, scanI2c);
  server.on("/api/uext/spi", HTTP_POST, transferSpi);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void connectWiFi() {
  // An empty SSID intentionally selects AP mode without fake credentials.
  bool credentialsSet = WIFI_SSID[0] != '\0';
  if (credentialsSet) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) { delay(250); Serial.print('.'); }
    Serial.println();
  }

  if (!credentialsSet || WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    accessPointMode = true;
    WiFi.softAP(AP_SSID);
    Serial.print("Access point: "); Serial.println(AP_SSID);
    Serial.print("Open http://"); Serial.println(WiFi.softAPIP());
  } else {
    Serial.print("Open http://"); Serial.println(WiFi.localIP());
  }

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS: http://"); Serial.print(MDNS_NAME); Serial.println(".local");
  }
}

void updateButton() {
  // Non-blocking debounce: act only on the stable pressed edge.
  bool rawPressed = digitalRead(USER_BUTTON_PIN) == LOW;
  unsigned long now = millis();
  if (rawPressed != lastRawButtonPressed) {
    lastRawButtonPressed = rawPressed;
    lastButtonChangeMs = now;
  }
  if (now - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS && rawPressed != buttonPressed) {
    buttonPressed = rawPressed;
    if (buttonPressed) runButtonStep();
  }
}

void updateUextUart() {
  // Preserve readable text; represent binary bytes as \xNN in the web terminal.
  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  while (uextUart.available()) {
    uint8_t value = uextUart.read();
    if (value == '\r' || value == '\n' || value == '\t' || (value >= 32 && value <= 126)) {
      uextUartRx += (char)value;
    } else {
      uextUartRx += F("\\x");
      uextUartRx += HEX_DIGITS[value >> 4];
      uextUartRx += HEX_DIGITS[value & 0x0F];
    }
  }
  if (uextUartRx.length() > 1024) uextUartRx.remove(0, uextUartRx.length() - 1024);
}

void setup() {
  Serial.begin(115200);
  unsigned long serialWaitStarted = millis();
  while (!Serial && millis() - serialWaitStarted < 3000) delay(10);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" OLIMEX ESP32-C6-EVB WEB SERVER DEMO"));
  Serial.println(F("========================================"));
  for (uint8_t i = 0; i < 4; ++i) { pinMode(RELAY_PINS[i], OUTPUT); setRelay(i, false); }
  pinMode(USER_LED_PIN, OUTPUT); setUserLed(false);
  pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
  for (uint8_t i = 0; i < 4; ++i) pinMode(OPTO_PINS[i], INPUT);
  uextUart.begin(UEXT_UART_BAUD, SERIAL_8N1, UEXT_UART_RX_PIN, UEXT_UART_TX_PIN);
  Wire.begin(UEXT_I2C_SDA_PIN, UEXT_I2C_SCL_PIN, UEXT_I2C_CLOCK);
  Wire.setTimeOut(50); // A faulty UEXT device must not block the web UI.
  pinMode(UEXT_SPI_CS_PIN, OUTPUT);
  digitalWrite(UEXT_SPI_CS_PIN, HIGH);
  uextSpi.begin(UEXT_SPI_SCK_PIN, UEXT_SPI_MISO_PIN, UEXT_SPI_MOSI_PIN, UEXT_SPI_CS_PIN);

  Serial.println(F("Hardware initialized: relays, LED, button, optocouplers and UEXT"));
  Serial.println(F("UEXT: UART GPIO5/4, I2C GPIO7/6, SPI GPIO20/18/19/21"));
  connectWiFi();
  configureWebServer();
  Serial.println(F("HTTP server started successfully."));
  printNetworkInfo();
  printHardwareStatus();
  lastSerialAddressReminderMs = millis();
}

void loop() {
  updateButton();
  updateUextUart();
  server.handleClient();
  if (millis() - lastSerialAddressReminderMs >= SERIAL_ADDRESS_REMINDER_MS) {
    lastSerialAddressReminderMs = millis();
    printNetworkInfo();
  }
  delay(1);
}
