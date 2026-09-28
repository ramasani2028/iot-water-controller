#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// Wi-Fi Credentials - Update for your network
// ==========================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ==========================================
// Pin Definitions (ESP32 GPIOs)
// ==========================================
const int LCD_RS = 2;
const int LCD_E  = 3;   // Note: GPIO15 or GPIO27 recommended on physical hardware
const int LCD_D4 = 4;
const int LCD_D5 = 16;
const int LCD_D6 = 17;
const int LCD_D7 = 5;

const int TRIG_PIN = 18;
const int ECHO_PIN = 19;
const int RELAY_PIN = 23;
const int BUTTON_PIN = 13;
const int MODE_SWITCH_PIN = 12;

// ==========================================
// Objects & Global Variables
// ==========================================
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
WebServer server(80);

int targetLevel = 100;    // Target distance in inches (stored in EEPROM)
int waterDistance = 0;   // Current distance in inches
int waterPercent = 0;    // Calculated water level percentage
bool pumpOn = false;     // Pump state (true = running, false = stopped)
bool buttonPressed = false;

const int LOW_THRESHOLD = 30;   // Start pump when level < 30%
const int HIGH_THRESHOLD = 95;  // Stop pump when level > 95%

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 500; // Read sensor every 500ms

// Function declarations
void connectWiFi();
void setupWebServer();
void sendJsonStatus();
void handleTogglePump();
void handleSetTarget();
int readDistanceInches();
int calculatePercentage(int target, int distance);
bool readButtonPressed();
void saveTargetLevel(int level);
void updateDisplay(bool autoMode);
String buildHtmlDashboard();

// ==========================================
// Setup
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  // Default relay state OFF (Active-Low relay: HIGH = OFF)
  digitalWrite(RELAY_PIN, HIGH);

  EEPROM.begin(512);
  targetLevel = EEPROM.read(0);
  if (targetLevel < 10 || targetLevel > 150) {
    targetLevel = 100; // Default fallback
  }

  lcd.begin(16, 2);
  lcd.print("WATER LEVEL IOT");
  lcd.setCursor(0, 1);
  lcd.print("Booting...");

  connectWiFi();
  setupWebServer();
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
  // Handle HTTP Server requests
  server.handleClient();

  // Read sensors periodically without blocking HTTP handler
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;

    waterDistance = readDistanceInches();
    waterPercent = calculatePercentage(targetLevel, waterDistance);
    waterPercent = constrain(waterPercent, 0, 100);

    bool autoMode = digitalRead(MODE_SWITCH_PIN) == HIGH;
    bool pressed = readButtonPressed();

    if (autoMode) {
      if (pressed) {
        saveTargetLevel(waterDistance);
      }
      if (waterPercent < LOW_THRESHOLD) {
        pumpOn = true;
      } else if (waterPercent >= HIGH_THRESHOLD) {
        pumpOn = false;
      }
    } else {
      if (pressed) {
        pumpOn = !pumpOn;
      }
    }

    // Active-low relay output
    digitalWrite(RELAY_PIN, !pumpOn);
    updateDisplay(autoMode);
  }
}

// ==========================================
// Wi-Fi Connection
// ==========================================
void connectWiFi() {
  lcd.clear();
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  lcd.print("SSID: ");
  lcd.print(ssid);

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());
  } else {
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Offline Mode");
    Serial.println("\nWi-Fi connection failed. Operating offline.");
  }
}

// ==========================================
// HTTP Web Server & REST API Routes
// ==========================================
void setupWebServer() {
  // Web Dashboard Route
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", buildHtmlDashboard());
  });

  // JSON API Endpoint
  server.on("/api/status", HTTP_GET, sendJsonStatus);
  server.on("/status", HTTP_GET, sendJsonStatus); // Alias endpoint

  // Control Endpoint: Toggle Pump
  server.on("/api/toggle", HTTP_POST, handleTogglePump);
  server.on("/toggle", HTTP_GET, handleTogglePump); // Fallback GET for easy browser URL test

  // Control Endpoint: Set Target Level
  server.on("/api/set-target", HTTP_POST, handleSetTarget);
  server.on("/set-target", HTTP_GET, handleSetTarget); // Fallback GET

  // Handle 404
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"Not Found\"}");
  });

  server.begin();
  Serial.println("HTTP Web Server started");
}

void sendJsonStatus() {
  bool autoMode = digitalRead(MODE_SWITCH_PIN) == HIGH;
  String json = "{";
  json += "\"mode\":\"" + String(autoMode ? "AUTO" : "MANUAL") + "\",";
  json += "\"pump\":\"" + String(pumpOn ? "ON" : "OFF") + "\",";
  json += "\"pump_boolean\":" + String(pumpOn ? "true" : "false") + ",";
  json += "\"water_percent\":" + String(waterPercent) + ",";
  json += "\"distance_in\":" + String(waterDistance) + ",";
  json += "\"target_in\":" + String(targetLevel) + ",";
  json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"uptime_sec\":" + String(millis() / 1000);
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleTogglePump() {
  bool autoMode = digitalRead(MODE_SWITCH_PIN) == HIGH;
  if (!autoMode) {
    pumpOn = !pumpOn;
    digitalWrite(RELAY_PIN, !pumpOn);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"success\":true,\"pump\":\"" + String(pumpOn ? "ON" : "OFF") + "\"}");
  } else {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Cannot toggle pump in AUTO mode\"}");
  }
}

void handleSetTarget() {
  saveTargetLevel(waterDistance);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"success\":true,\"target_in\":" + String(targetLevel) + "}");
}

// ==========================================
// Sensor & Utility Functions
// ==========================================
int readDistanceInches() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long durationUs = pulseIn(ECHO_PIN, HIGH, 30000);
  if (durationUs == 0) return 0;
  return durationUs / 74 / 2; // Conversion to inches
}

int calculatePercentage(int target, int distance) {
  if (target <= 0) return 0;
  return (target - distance) * 100 / target;
}

bool readButtonPressed() {
  bool current = digitalRead(BUTTON_PIN) == LOW;
  if (current && !buttonPressed) {
    buttonPressed = true;
    return true;
  }
  if (!current) {
    buttonPressed = false;
  }
  return false;
}

void saveTargetLevel(int level) {
  targetLevel = constrain(level, 10, 150);
  EEPROM.write(0, targetLevel);
  EEPROM.commit();
  Serial.print("Target level saved: ");
  Serial.println(targetLevel);
}

void updateDisplay(bool autoMode) {
  lcd.clear();
  lcd.print("WL:");
  lcd.print(waterPercent);
  lcd.print("% ");
  lcd.setCursor(0, 1);
  lcd.print(autoMode ? "AUTO " : "MANUAL");
  lcd.print(pumpOn ? " PUMP ON" : " PUMP OFF");
}

// ==========================================
// Responsive Web Dashboard (HTML + CSS + JS)
// ==========================================
String buildHtmlDashboard() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>IoT Water Controller Dashboard</title>
  <style>
    * { box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { background: #f0f4f8; margin: 0; padding: 20px; color: #333; }
    .card { max-width: 480px; margin: 0 auto; background: #fff; padding: 24px; border-radius: 16px; box-shadow: 0 10px 25px rgba(0,0,0,0.08); }
    h2 { margin-top: 0; color: #0284c7; text-align: center; font-size: 24px; }
    .status-badge { display: inline-block; padding: 6px 14px; border-radius: 20px; font-weight: bold; font-size: 14px; }
    .badge-on { background: #dcfce7; color: #15803d; }
    .badge-off { background: #fee2e2; color: #b91c1c; }
    .badge-auto { background: #e0f2fe; color: #0369a1; }
    .badge-manual { background: #fef3c7; color: #b45309; }
    
    .gauge-container { margin: 24px 0; background: #e2e8f0; border-radius: 12px; height: 32px; overflow: hidden; position: relative; }
    .gauge-fill { height: 100%; background: linear-gradient(90deg, #38bdf8, #0284c7); width: 0%; transition: width 0.5s ease-in-out; }
    .gauge-text { position: absolute; width: 100%; text-align: center; top: 0; line-height: 32px; font-weight: bold; color: #1e293b; text-shadow: 0 0 4px rgba(255,255,255,0.8); }
    
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 20px; }
    .stat-box { background: #f8fafc; padding: 12px; border-radius: 8px; text-align: center; border: 1px solid #e2e8f0; }
    .stat-val { font-size: 20px; font-weight: bold; color: #0f172a; }
    .stat-lbl { font-size: 12px; color: #64748b; text-transform: uppercase; margin-top: 4px; }

    .btn-group { display: flex; flex-direction: column; gap: 10px; }
    button { padding: 12px; border: none; border-radius: 8px; font-size: 15px; font-weight: bold; cursor: pointer; transition: 0.2s; }
    .btn-primary { background: #0284c7; color: white; }
    .btn-primary:hover { background: #0369a1; }
    .btn-secondary { background: #64748b; color: white; }
    .btn-secondary:hover { background: #475569; }
    button:disabled { background: #cbd5e1; cursor: not-allowed; }

    .footer { margin-top: 16px; text-align: center; font-size: 12px; color: #94a3b8; }
  </style>
</head>
<body>
  <div class="card">
    <h2>🌊 IoT Water Controller</h2>
    
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
      <div>Mode: <span id="mode-badge" class="status-badge badge-auto">AUTO</span></div>
      <div>Pump: <span id="pump-badge" class="status-badge badge-off">OFF</span></div>
    </div>

    <div class="gauge-container">
      <div id="gauge-fill" class="gauge-fill"></div>
      <div id="gauge-text" class="gauge-text">0%</div>
    </div>

    <div class="grid">
      <div class="stat-box">
        <div id="distance-val" class="stat-val">-- in</div>
        <div class="stat-lbl">Current Distance</div>
      </div>
      <div class="stat-box">
        <div id="target-val" class="stat-val">-- in</div>
        <div class="stat-lbl">Target Reference</div>
      </div>
    </div>

    <div class="btn-group">
      <button id="toggle-btn" class="btn-primary" onclick="togglePump()">Toggle Pump (Manual Mode)</button>
      <button class="btn-secondary" onclick="setTarget()">Calibrate Target Level</button>
    </div>

    <div class="footer">
      Live update via ESP32 REST API &bull; RSSI: <span id="rssi">--</span> dBm
    </div>
  </div>

  <script>
    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        document.getElementById('mode-badge').innerText = data.mode;
        document.getElementById('mode-badge').className = 'status-badge ' + (data.mode === 'AUTO' ? 'badge-auto' : 'badge-manual');
        
        document.getElementById('pump-badge').innerText = data.pump;
        document.getElementById('pump-badge').className = 'status-badge ' + (data.pump === 'ON' ? 'badge-on' : 'badge-off');
        
        document.getElementById('gauge-fill').style.width = data.water_percent + '%';
        document.getElementById('gauge-text').innerText = data.water_percent + '% Full';
        
        document.getElementById('distance-val').innerText = data.distance_in + ' in';
        document.getElementById('target-val').innerText = data.target_in + ' in';
        document.getElementById('rssi').innerText = data.wifi_rssi;
        
        document.getElementById('toggle-btn').disabled = (data.mode === 'AUTO');
      } catch (err) {
        console.error('Failed to fetch status:', err);
      }
    }

    async function togglePump() {
      try {
        await fetch('/api/toggle', { method: 'POST' });
        updateStatus();
      } catch (err) {
        console.error('Failed to toggle pump:', err);
      }
    }

    async function setTarget() {
      try {
        await fetch('/api/set-target', { method: 'POST' });
        alert('Target water level saved!');
        updateStatus();
      } catch (err) {
        console.error('Failed to set target:', err);
      }
    }

    setInterval(updateStatus, 2000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
  return html;
}
