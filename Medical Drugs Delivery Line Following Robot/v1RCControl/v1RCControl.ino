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

WebServer server(80);

// Fungsi kendali motor
void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);  // Roda kanan
  ledcWrite(ENB, 0);  // Roda kiri
}

void maju() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // Kanan maju
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); // Kiri maju
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
}

void mundur() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // Kanan mundur
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); // Kiri mundur
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
}

void kanan() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // Kanan mundur
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); // Kiri maju
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
}

void kiri() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // Kanan maju
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); // Kiri mundur
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
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

  // Pin output
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Attach PWM sesuai ESP32 Core 3.x
  ledcAttach(ENA, 1000, 8);  // pin 13
  ledcAttach(ENB, 1000, 8);  // pin 14

  stopMotor();

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP siap, IP: ");
  Serial.println(WiFi.softAPIP());

  // Routing
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/maju", []() { maju(); server.send(200, "text/plain", "OK"); });
  server.on("/mundur", []() { mundur(); server.send(200, "text/plain", "OK"); });
  server.on("/kanan", []() { kanan(); server.send(200, "text/plain", "OK"); });
  server.on("/kiri", []() { kiri(); server.send(200, "text/plain", "OK"); });
  server.on("/stop", []() { stopMotor(); server.send(200, "text/plain", "OK"); });

  server.begin();
}

void loop() {
  server.handleClient();
}
