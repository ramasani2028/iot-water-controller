#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const int LCD_RS = 2;
const int LCD_E = 3;
const int LCD_D4 = 4;
const int LCD_D5 = 16;
const int LCD_D6 = 17;
const int LCD_D7 = 5;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

const int TRIG_PIN = 18;
const int ECHO_PIN = 19;
const int RELAY_PIN = 23;
const int BUTTON_PIN = 13;
const int MODE_SWITCH_PIN = 12;

WebServer server(80);

int targetLevel = 100;
int waterDistance = 0;
int waterPercent = 0;
bool pumpOn = false;
bool buttonPressed = false;

const int LOW_THRESHOLD = 30;
const int HIGH_THRESHOLD = 95;

long durationUs;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, HIGH); // keep relay off for active-low modules

  EEPROM.begin(512);
  targetLevel = EEPROM.read(0);
  if (targetLevel < 10 || targetLevel > 150) {
    targetLevel = 100;
  }

  lcd.begin(16, 2);
  lcd.print("WATER LEVEL IOT");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  connectWiFi();
  setupWebServer();
}

void loop() {
  waterDistance = readDistanceInches();
  waterPercent = calculatePercentage(targetLevel, waterDistance);
  if (waterPercent < 0) waterPercent = 0;
  if (waterPercent > 100) waterPercent = 100;

  bool autoMode = digitalRead(MODE_SWITCH_PIN) == HIGH;
  bool pressed = readButtonPressed();

  if (autoMode) {
    if (pressed) {
      saveTargetLevel(waterDistance);
    }
    if (waterPercent < LOW_THRESHOLD) {
      pumpOn = true;
    } else if (waterPercent > HIGH_THRESHOLD) {
      pumpOn = false;
    }
  } else {
    if (pressed) {
      pumpOn = !pumpOn;
    }
  }

  digitalWrite(RELAY_PIN, !pumpOn);
  updateDisplay(autoMode);
  server.handleClient();
  delay(250);
}

void connectWiFi() {
  lcd.clear();
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Check credentials");
    Serial.println("WiFi connection failed");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", buildHtmlPage());
  });

  server.on("/toggle", HTTP_GET, []() {
    if (digitalRead(MODE_SWITCH_PIN) == LOW) {
      pumpOn = !pumpOn;
      digitalWrite(RELAY_PIN, !pumpOn);
      server.send(200, "text/plain", "Pump toggled");
    } else {
      server.send(400, "text/plain", "Use manual mode to toggle pump");
    }
  });

  server.on("/set-target", HTTP_GET, []() {
    saveTargetLevel(waterDistance);
    server.send(200, "text/plain", "Target level saved");
  });

  server.on("/status", HTTP_GET, []() {
    String body = "{";
    body += "\"mode\":";
    body += digitalRead(MODE_SWITCH_PIN) == HIGH ? "\"AUTO\"" : "\"MANUAL\"";
    body += ",\"pump\":";
    body += pumpOn ? "\"ON\"" : "\"OFF\"";
    body += ",\"water_percent\":";
    body += waterPercent;
    body += ",\"distance_in\":";
    body += waterDistance;
    body += ",\"target_in\":";
    body += targetLevel;
    body += "}";
    server.send(200, "application/json", body);
  });

  server.begin();
  Serial.println("Web server started");
}

int readDistanceInches() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  durationUs = pulseIn(ECHO_PIN, HIGH, 30000);
  if (durationUs == 0) {
    return 0;
  }
  return durationUs / 74 / 2;
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
  Serial.print("Saved target level: ");
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

String buildHtmlPage() {
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>IoT Water Controller</title>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:500px;margin:auto;padding:20px;}h1{color:#2a6eb5;}button{padding:10px 18px;margin:6px;border:none;background:#2a6eb5;color:white;cursor:pointer;}button:disabled{background:#999;}</style>";
  html += "</head><body><h1>IoT Water Controller</h1>";
  html += "<p><strong>Mode:</strong> ";
  html += digitalRead(MODE_SWITCH_PIN) == HIGH ? "AUTO" : "MANUAL";
  html += "</p><p><strong>Pump:</strong> ";
  html += pumpOn ? "ON" : "OFF";
  html += "</p><p><strong>Water Level:</strong> ";
  html += waterPercent;
  html += "%</p><p><strong>Distance:</strong> ";
  html += waterDistance;
  html += " in</p><p><strong>Target:</strong> ";
  html += targetLevel;
  html += " in</p>";
  if (digitalRead(MODE_SWITCH_PIN) == LOW) {
    html += "<p><a href=\"/toggle\"><button>Toggle Pump</button></a></p>";
  } else {
    html += "<p><button disabled>Toggle Pump (Auto mode)</button></p>";
  }
  html += "<p><a href=\"/set-target\"><button>Save Current Level</button></a></p>";
  html += "<p><a href=\"/status\">View JSON Status</a></p>";
  html += "</body></html>";
  return html;
}
