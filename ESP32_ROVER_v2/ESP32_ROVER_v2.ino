#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ─── Motor Pins ───────────────────────────────────────────────
#define IN1 26   // Motor A
#define IN2 27
#define IN3 14   // Motor B
#define IN4 12

// ─── Hotspot Credentials ──────────────────────────────────────
const char* AP_SSID = "ESP32_Robot";
const char* AP_PASS = "12345678";

AsyncWebServer server(80);
AsyncWebSocket  ws("/ws");

// ─── HTML + CSS + JS (Virtual Joystick) ──────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no"/>
  <title>ESP32 Robot Controller</title>
  <style>
    * { margin:0; padding:0; box-sizing:border-box; }
    body {
      background: #0d0d1a;
      display: flex; flex-direction: column;
      align-items: center; justify-content: center;
      height: 100vh;
      font-family: 'Segoe UI', Arial, sans-serif;
      color: #fff;
      touch-action: none;
      overflow: hidden;
    }
    h1 { font-size: 1.6rem; color: #e94560; margin-bottom: 8px; letter-spacing: 2px; }
    #ws-status {
      font-size: 0.85rem; color: #888;
      margin-bottom: 8px;
    }
    #dir-label {
      font-size: 1.1rem; font-weight: bold;
      background: #e94560; color: #fff;
      padding: 6px 24px; border-radius: 20px;
      margin-bottom: 30px;
      min-width: 160px; text-align: center;
      letter-spacing: 1px;
    }
    #joy-base {
      position: relative;
      width: 220px; height: 220px;
      background: rgba(255,255,255,0.07);
      border-radius: 50%;
      border: 2.5px solid #e94560;
      box-shadow: 0 0 30px rgba(233,69,96,0.25);
    }
    /* Crosshair lines */
    #joy-base::before, #joy-base::after {
      content: '';
      position: absolute;
      background: rgba(255,255,255,0.08);
    }
    #joy-base::before { width: 2px; height: 100%; left: 50%; transform: translateX(-50%); }
    #joy-base::after  { height: 2px; width: 100%; top: 50%; transform: translateY(-50%); }
    #joy-knob {
      position: absolute;
      width: 84px; height: 84px;
      background: radial-gradient(circle at 35% 35%, #ff6b81, #c62a47);
      border-radius: 50%;
      top: 50%; left: 50%;
      transform: translate(-50%, -50%);
      cursor: grab;
      box-shadow: 0 0 20px rgba(233,69,96,0.6);
      transition: box-shadow 0.1s;
      z-index: 2;
    }
    #joy-knob:active { cursor: grabbing; box-shadow: 0 0 35px rgba(233,69,96,0.9); }
    .info-row {
      margin-top: 22px;
      font-size: 0.78rem;
      color: #555;
      text-align: center;
    }
  </style>
</head>
<body>

<h1>&#x1F916; ESP32 ROBOT</h1>
<div id="ws-status">Connecting...</div>
<div id="dir-label">STOP</div>

<div id="joy-base">
  <div id="joy-knob"></div>
</div>

<div class="info-row">
  Connect to Wi-Fi: <b>ESP32_Robot</b> &nbsp;|&nbsp; IP: 192.168.4.1
</div>

<script>
  const wsConn   = new WebSocket('ws://' + location.host + '/ws');
  const statusEl = document.getElementById('ws-status');
  const dirEl    = document.getElementById('dir-label');
  const base     = document.getElementById('joy-base');
  const knob     = document.getElementById('joy-knob');

  wsConn.onopen  = () => statusEl.textContent = '✅ Connected';
  wsConn.onclose = () => { statusEl.textContent = '❌ Disconnected — Refresh page'; };

  const MAX_R = 68;
  let dragging = false;

  function getCenter() {
    const r = base.getBoundingClientRect();
    return { x: r.left + r.width / 2, y: r.top + r.height / 2 };
  }

  function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

  function moveKnob(cx, cy) {
    const c   = getCenter();
    let dx    = cx - c.x, dy = cy - c.y;
    const d   = Math.sqrt(dx*dx + dy*dy);
    if (d > MAX_R) { dx = dx/d*MAX_R; dy = dy/d*MAX_R; }
    knob.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
    const jx = Math.round(clamp(dx / MAX_R * 100, -100, 100));
    const jy = Math.round(clamp(-dy / MAX_R * 100, -100, 100));
    sendXY(jx, jy);
    updateLabel(jx, jy);
  }

  function resetKnob() {
    knob.style.transform = 'translate(-50%, -50%)';
    sendXY(0, 0);
    dirEl.textContent = 'STOP';
  }

  function sendXY(x, y) {
    if (wsConn.readyState === WebSocket.OPEN) wsConn.send(x + ',' + y);
  }

  function updateLabel(x, y) {
    const D = 25;
    const fwd = y > D, bwd = y < -D, rgt = x > D, lft = x < -D;
    if (!fwd && !bwd && !rgt && !lft)  dirEl.textContent = 'STOP';
    else if (fwd && !rgt && !lft)      dirEl.textContent = '⬆ FORWARD';
    else if (bwd && !rgt && !lft)      dirEl.textContent = '⬇ BACKWARD';
    else if (rgt && !fwd && !bwd)      dirEl.textContent = '➡ SPIN RIGHT';
    else if (lft && !fwd && !bwd)      dirEl.textContent = '⬅ SPIN LEFT';
    else if (fwd && rgt)               dirEl.textContent = '↗ FWD-RIGHT';
    else if (fwd && lft)               dirEl.textContent = '↖ FWD-LEFT';
    else if (bwd && rgt)               dirEl.textContent = '↘ BWD-RIGHT';
    else if (bwd && lft)               dirEl.textContent = '↙ BWD-LEFT';
  }

  // ── Mouse events ─────────────────────────────────────────────
  knob.addEventListener('mousedown',  e => { dragging = true; e.preventDefault(); });
  document.addEventListener('mousemove', e => { if (dragging) moveKnob(e.clientX, e.clientY); });
  document.addEventListener('mouseup',   () => { if (dragging) { dragging = false; resetKnob(); } });

  // ── Touch events ─────────────────────────────────────────────
  knob.addEventListener('touchstart', e => { dragging = true; e.preventDefault(); }, { passive: false });
  document.addEventListener('touchmove', e => {
    if (dragging) { moveKnob(e.touches[0].clientX, e.touches[0].clientY); e.preventDefault(); }
  }, { passive: false });
  document.addEventListener('touchend', () => { if (dragging) { dragging = false; resetKnob(); } });
</script>
</body>
</html>
)rawliteral";


// ─── Motor Helper ─────────────────────────────────────────────
void setMotors(bool a1, bool a2, bool b1, bool b2) {
  digitalWrite(IN1, a1);
  digitalWrite(IN2, a2);
  digitalWrite(IN3, b1);
  digitalWrite(IN4, b2);
}

// ─── Joystick → Motor Logic ───────────────────────────────────
void handleJoystick(int x, int y) {
  const int DEAD = 25;
  bool fwd = y >  DEAD;
  bool bwd = y < -DEAD;
  bool rgt = x >  DEAD;
  bool lft = x < -DEAD;

  // Motor A = Left wheel,  Motor B = Right wheel
  if      (fwd && !rgt && !lft)  setMotors(1,0, 1,0);  // Forward
  else if (bwd && !rgt && !lft)  setMotors(0,1, 0,1);  // Backward
  else if (rgt && !fwd && !bwd)  setMotors(1,0, 0,1);  // Spin Right
  else if (lft && !fwd && !bwd)  setMotors(0,1, 1,0);  // Spin Left
  else if (fwd && rgt)           setMotors(1,0, 0,0);  // Fwd-Right (left motor only)
  else if (fwd && lft)           setMotors(0,0, 1,0);  // Fwd-Left  (right motor only)
  else if (bwd && rgt)           setMotors(0,1, 0,0);  // Bwd-Right
  else if (bwd && lft)           setMotors(0,0, 0,1);  // Bwd-Left
  else                           setMotors(0,0, 0,0);  // Stop
}

// ─── WebSocket Event Handler ──────────────────────────────────
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%u disconnected\n", client->id());
    setMotors(0,0,0,0);  // Safety stop on disconnect
  } else if (type == WS_EVT_DATA) {
    String msg = "";
    for (size_t i = 0; i < len; i++) msg += (char)data[i];
    int comma = msg.indexOf(',');
    if (comma > 0) {
      int x = msg.substring(0, comma).toInt();
      int y = msg.substring(comma + 1).toInt();
      handleJoystick(x, y);
    }
  }
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  setMotors(0, 0, 0, 0);  // Safe start

  // Start Hotspot
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("Hotspot started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());  // Should print 192.168.4.1

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve webpage
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();
  Serial.println("HTTP server started");
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  ws.cleanupClients();  // Free disconnected clients
}
