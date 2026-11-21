#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#define ADXL_SDA     21
#define ADXL_SCL     22
#define SIM808_RX    16
#define SIM808_TX    17
#define BUZZER_PIN   25

#define FALL_THRESHOLD_G     1.8f
#define FREEFALL_THRESHOLD_G 0.55f
#define FREEFALL_WINDOW_MS   1200UL
#define ALERT_COOLDOWN_MS    30000UL

const char PHONE[] = "+918122735200";

HardwareSerial sim808(2);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

bool adxlReady = false;
bool freefallSeen = false;
bool gpsStarted = false;
bool fallLatched = false;

unsigned long freefallTime = 0;
unsigned long lastAlertTime = 0;
unsigned long buzzerUntil = 0;

float currentG = 1.0f;

// ---------- MODEM HELPERS ----------

String readModem(unsigned long timeout) {
  unsigned long start = millis();
  String data;
  while (millis() - start < timeout) {
    while (sim808.available()) {
      data += (char)sim808.read();
    }
  }
  return data;
}

String sendAT(const char* cmd, unsigned long timeout = 1000) {
  sim808.println(cmd);
  String resp = readModem(timeout);
  Serial.print("[AT] ");
  Serial.print(cmd);
  Serial.print(" -> ");
  Serial.println(resp);
  return resp;
}

bool waitForPrompt(unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (sim808.available()) {
      char c = sim808.read();
      if (c == '>') return true;
    }
  }
  return false;
}

// ---------- GPS ----------

bool getGPS(String &latitude, String &longitude, String &utcTime) {
  String response = sendAT("AT+CGNSINF", 2500);
  int pos = response.indexOf("+CGNSINF:");
  if (pos < 0) {
    return false;
  }
  int end = response.indexOf('\n', pos);
  String line = response.substring(
    pos + String("+CGNSINF:").length(),
    end < 0 ? response.length() : end
  );
  line.trim();
  String fields[20];
  int count = 0;
  int start = 0;
  for (int i = 0; i <= line.length() && count < 20; i++) {
    if (i == line.length() || line[i] == ',') {
      fields[count++] = line.substring(start, i);
      start = i + 1;
    }
  }
  // SIM808 CGNSINF:
  // 0=GNSS run status, 1=fix status, 2=UTC,
  // 3=latitude, 4=longitude
  if (count < 5 || fields[1] != "1") {
    return false;
  }
  utcTime = fields[2];
  latitude = fields[3];
  longitude = fields[4];
  if (latitude.length() == 0 || longitude.length() == 0) {
    return false;
  }
  if (latitude == "0.000000" && longitude == "0.000000") {
    return false;
  }
  return true;
}

// ---------- SMS ----------

bool sendSMS(String message) {
  Serial.println("\n--- Sending emergency SMS ---");
  sendAT("AT+CMGF=1", 1500);
  while (sim808.available()) {
    sim808.read();
  }
  Serial.println(">> AT+CMGS");
  sim808.print("AT+CMGS=\"");
  sim808.print(PHONE);
  sim808.println("\"");
  if (!waitForPrompt(5000)) {
    Serial.println("\nERROR: SMS prompt not received.");
    return false;
  }
  sim808.print(message);
  sim808.write(26); // Ctrl+Z submits SMS
  String result = readModem(20000);
  if (result.indexOf("+CMGS:") >= 0 &&
      result.indexOf("ERROR") < 0) {
    Serial.println("\nSMS submission accepted.");
    return true;
  }
  Serial.println("\nSMS submission failed or unconfirmed.");
  return false;
}

// ---------- ALERT ----------

void triggerFallAlert(float impactG) {
  unsigned long now = millis();
  if (fallLatched ||
      (lastAlertTime != 0 &&
       now - lastAlertTime < ALERT_COOLDOWN_MS)) {
    return;
  }
  fallLatched = true;
  lastAlertTime = now;
  buzzerUntil = now + 10000UL;
  Serial.println("\n!!! POSSIBLE FALL DETECTED !!!");
  Serial.print("Impact: ");
  Serial.print(impactG, 2);
  Serial.println(" g");
  digitalWrite(BUZZER_PIN, HIGH);

  String lat, lon, utc;
  bool gpsFix = false;
  // Allow time for GPS to acquire a fix.
  for (int attempt = 0; attempt < 3; attempt++) {
    if (getGPS(lat, lon, utc)) {
      gpsFix = true;
      break;
    }
    Serial.println("GPS fix not available yet.");
    delay(1500);
  }

  String message = "FALL-SAFE SAFETY ALERT!\n";
  message += "Possible worker fall detected.\n";
  message += "Impact: ";
  message += String(impactG, 2);
  message += " g\n";

  if (gpsFix) {
    String mapLink = "https://maps.google.com/?q=" + lat + "," + lon;
    message += "GPS UTC: ";
    message += utc;
    message += "\nLatitude: ";
    message += lat;
    message += "\nLongitude: ";
    message += lon;
    message += "\nLocation: ";
    message += mapLink;
  } else {
    message += "GPS location unavailable. Check worker location manually.";
  }

  sendSMS(message);

  Serial.println("\nAlert routine complete.");
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(ADXL_SDA, ADXL_SCL);

  Serial.println("\nFALL-SAFE starting...");

  if (accel.begin(0x53)) {
    adxlReady = true;
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("ADXL345 detected at 0x53.");
  } else if (accel.begin(0x1D)) {
    adxlReady = true;
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("ADXL345 detected at 0x1D.");
  } else {
    Serial.println("ERROR: ADXL345 not detected.");
    Serial.println("Check VCC, GND, SDA, SCL, SDO and CS.");
  }

  sim808.begin(9600, SERIAL_8N1, SIM808_RX, SIM808_TX);
  delay(3000);
  String at = sendAT("AT", 2000);
  if (at.indexOf("OK") >= 0) {
    sendAT("ATE0");
    sendAT("AT+CPIN?");
    sendAT("AT+CSQ");
    sendAT("AT+CREG?");
    sendAT("AT+CMGF=1");
    String gpsPower = sendAT("AT+CGNSPWR=1", 2000);
    gpsStarted = gpsPower.indexOf("OK") >= 0;
    Serial.println("SIM808 communication established.");
  } else {
    Serial.println("ERROR: SIM808 did not respond.");
    Serial.println("Check module power, wiring and baud rate.");
  }

  Serial.println("\nFALL-SAFE ready.");
  Serial.println("Keep the sensor still briefly before testing.");
}

// ---------- MAIN LOOP ----------

void loop() {
  unsigned long now = millis();
  if (now >= buzzerUntil) {
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (!adxlReady) {
    delay(500);
    return;
  }

  sensors_event_t event;
  accel.getEvent(&event);
  float ax = event.acceleration.x / 9.80665f;
  float ay = event.acceleration.y / 9.80665f;
  float az = event.acceleration.z / 9.80665f;
  currentG = sqrtf(ax * ax + ay * ay + az * az);

  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 250) {
    lastPrint = now;
    Serial.print("Acceleration magnitude: ");
    Serial.print(currentG, 2);
    Serial.println(" g");
  }

  if (currentG < FREEFALL_THRESHOLD_G) {
    freefallSeen = true;
    freefallTime = now;
  }

  if (freefallSeen) {
    if (now - freefallTime > FREEFALL_WINDOW_MS) {
      freefallSeen = false;
    } else if (currentG >= FALL_THRESHOLD_G) {
      freefallSeen = false;
      triggerFallAlert(currentG);
    }
  } else {
    calmSince = 0;
  }

  delay(20);
}
