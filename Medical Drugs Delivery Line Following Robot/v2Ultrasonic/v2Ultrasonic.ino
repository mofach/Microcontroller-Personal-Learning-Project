#include <WiFi.h>
#include <WebServer.h>

// SSID dan password WiFi Access Point
const char* ssid = "MobilRC";
const char* password = "12345678";

// Motor kanan
#define ENA 13
#define IN1 32
#define IN2 33

// Motor kiri
#define ENB 14
#define IN3 25
#define IN4 26

// Sensor ultrasonic
#define TRIG 27
#define ECHO 4

// Sensor buzzer
#define BUZZER 2

WebServer server(80);
bool blokirMaju = false;

// Fungsi kendali motor
void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

void maju() {
  if (blokirMaju) return;
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
}

void mundur() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
  blokirMaju = false; // reset blokir
}

void kanan() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
  blokirMaju = false;
}

void kiri() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
  blokirMaju = false;
}

// Baca jarak ultrasonik
long bacaJarak() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long durasi = pulseIn(ECHO, HIGH, 30000); // timeout 30ms
  return durasi * 0.034 / 2;
}

// Halaman Web UI
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RC Control</title>
  <style>
    button {
      width: 100px; height: 60px; font-size: 18px; margin: 5px;
    }
  </style>
</head>
<body style="text-align:center;">
  <h2>Kendali Mobil RC</h2>
  <button onmousedown="fetch('/maju')" onmouseup="fetch('/stop')"
          ontouchstart="fetch('/maju')" ontouchend="fetch('/stop')">MAJU</button><br><br>

  <button onmousedown="fetch('/kiri')" onmouseup="fetch('/stop')"
          ontouchstart="fetch('/kiri')" ontouchend="fetch('/stop')">KIRI</button>

  <button onmousedown="fetch('/kanan')" onmouseup="fetch('/stop')"
          ontouchstart="fetch('/kanan')" ontouchend="fetch('/stop')">KANAN</button><br><br>

  <button onmousedown="fetch('/mundur')" onmouseup="fetch('/stop')"
          ontouchstart="fetch('/mundur')" ontouchend="fetch('/stop')">MUNDUR</button>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Setup pin motor
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Setup pin sensor & buzzer
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);

  // PWM Motor
  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);

  stopMotor();

  // WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Routing
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/maju", []() { maju(); server.send(200, "text/plain", "OK"); });
  server.on("/mundur", []() { mundur(); server.send(200, "text/plain", "OK"); });
  server.on("/kanan", []() { kanan(); server.send(200, "text/plain", "OK"); });
  server.on("/kiri", []() { kiri(); server.send(200, "text/plain", "OK"); });
  server.on("/stop", []() { stopMotor(); server.send(200, "text/plain", "STOP"); });

  server.begin();
}

void loop() {
  server.handleClient();

  // Deteksi halangan
  long jarak = bacaJarak();
  if (jarak > 0 && jarak <= 15) {
    digitalWrite(BUZZER, HIGH);
    blokirMaju = true;
    if (digitalRead(IN2) == HIGH && digitalRead(IN4) == HIGH) {
      stopMotor(); // paksa stop jika nekat maju
    }
  } else {
    digitalWrite(BUZZER, LOW);
  }
}
