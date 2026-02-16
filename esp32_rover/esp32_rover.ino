#include <WiFi.h>
#include <WebServer.h>

// ------------------- STA Credentials -------------------
const char* ssid     = "home";
const char* password = "idontknow4321";

// ------------------- AP Credentials (Fallback) -------------------
const char* ap_ssid     = "ESP32_BOT";
const char* ap_password = "12345678";   // Minimum 8 chars

// ------------------- Static IP (STA Mode) -------------------
IPAddress local_IP(192, 168, 1, 124);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);

// ------------------- AP IP Config -------------------
IPAddress ap_IP(192, 168, 4, 1);
IPAddress ap_gateway(192, 168, 4, 1);
IPAddress ap_subnet(255, 255, 255, 0);

// ------------------- Motor Pins -------------------
const int ENA = 33;  // Motor A PWM
const int IN1 = 12;  // Motor A direction 1
const int IN2 = 14;  // Motor A direction 2
const int IN3 = 27;  // Motor B direction 1
const int IN4 = 26;  // Motor B direction 2
const int ENB = 25;  // Motor B PWM

// ------------------- Dual PWM Controls -------------------
int motorSpeedA = 150;  // Left motor speed (0-255)
int motorSpeedB = 150;  // Right motor speed (0-255)

const int freq       = 30000;  // Hz
const int resolution = 8;      // bits (0-255)

// ------------------- Web Server -------------------
WebServer server(80);
String deviceIP = "";

// ------------------- HTML Page -------------------
String generateHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Dual Motor Control</title>
  <style>
    body { background:#222; color:white; text-align:center; font-family:Arial; padding:20px; }
    h2 { color:#0f0; }
    .slider { width:220px; height:30px; margin:10px; }
    .speed { font-size:18px; margin:5px; }
    button { padding:12px 20px; margin:8px; background:#444; color:white; border:none; border-radius:8px; font-size:16px; }
    button:hover { background:#666; }
    .controls { margin:20px 0; }
    .mode { background:#0066cc; padding:10px; border-radius:5px; margin-bottom:15px; }
  </style>
</head>
<body>
)rawliteral";

  html += "<h2>ESP32 Bot Control - " + deviceIP + "</h2>";

  html += "<div class=\"mode\">Mode: ";
  html += (WiFi.getMode() == WIFI_AP ? "Access Point" : "Station");
  html += "</div>";

  html += R"rawliteral(
  <div>
    <label class="speed">Left Motor:</label>
    <input type="range" class="slider" id="speedA" min="0" max="255" value="150"
           onchange="updateSpeed('A', this.value)">
    <span id="valA">150</span>
  </div>

  <div>
    <label class="speed">Right Motor:</label>
    <input type="range" class="slider" id="speedB" min="0" max="255" value="150"
           onchange="updateSpeed('B', this.value)">
    <span id="valB">150</span>
  </div>

  <div class="controls">
    <button onclick="sendCmd('fwd')">FWD</button>
    <button onclick="sendCmd('left')">LEFT</button>
    <button onclick="sendCmd('stop')">STOP</button>
    <button onclick="sendCmd('right')">RIGHT</button>
    <button onclick="sendCmd('bwd')">BWD</button>
  </div>

  <script>
    function sendCmd(cmd) {
      fetch('/action?go=' + cmd).then(() => console.log(cmd + ' sent'));
    }
    function updateSpeed(motor, value) {
      document.getElementById('val' + motor).innerText = value;
      fetch('/action?spd' + motor + '=' + value).then(() => console.log('Speed ' + motor + ':' + value));
    }
  </script>
</body>
</html>
)rawliteral";

  return html;
}

// ------------------- WiFi Setup -------------------
void setupWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    deviceIP = WiFi.localIP().toString();
    Serial.println("\nConnected STA: " + deviceIP);
  } else {
    Serial.println("\nSTA failed. Starting AP...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(ap_IP, ap_gateway, ap_subnet);
    WiFi.softAP(ap_ssid, ap_password);
    deviceIP = WiFi.softAPIP().toString();
    Serial.println("AP IP: " + deviceIP);
  }
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Motor Bot Starting...");

  setupWiFi();

  // Direction pins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // NEW LEDC API (ESP32 core 3.x): attach directly to pin
  ledcAttach(ENA, freq, resolution);   // Motor A PWM
  ledcAttach(ENB, freq, resolution);   // Motor B PWM

  // Initial duty cycles
  ledcWrite(ENA, motorSpeedA);
  ledcWrite(ENB, motorSpeedB);

  // Root page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });

  // Action handler
  server.on("/action", HTTP_GET, []() {
    // Handle independent speeds
    if (server.hasArg("spdA")) {
      motorSpeedA = constrain(server.arg("spdA").toInt(), 0, 255);
      ledcWrite(ENA, motorSpeedA);
    }
    if (server.hasArg("spdB")) {
      motorSpeedB = constrain(server.arg("spdB").toInt(), 0, 255);
      ledcWrite(ENB, motorSpeedB);
    }

    // Handle motion commands
    if (server.hasArg("go")) {
      String action = server.arg("go");

      if (action == "fwd") {
        // Both forward
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      }
      else if (action == "bwd") {
        // Both backward
        digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
      }
      else if (action == "left") {
        // Left motor backward, right motor forward
        digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      }
      else if (action == "right") {
        // Left motor forward, right motor backward
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
      }
      else if (action == "stop") {
        // Coast stop
        digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      }

      Serial.printf("Cmd: %s | A:%d B:%d\n",
                    action.c_str(), motorSpeedA, motorSpeedB);
    }

    // Ensure PWM is applied after any change
    ledcWrite(ENA, motorSpeedA);
    ledcWrite(ENB, motorSpeedB);

    server.send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("WebServer started.");
}

// ------------------- Loop -------------------
void loop() {
  server.handleClient();
}
