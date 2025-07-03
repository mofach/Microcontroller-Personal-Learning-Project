#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "esp_task_wdt.h"

// WiFi AP
const char* ssid = "ServoRC";
const char* password = "12345678";
WebServer server(80);

// Pin servo
#define SERVO_ENGSEL 16
#define SERVO_KUNCI 17

Servo engsel, kunci;

bool bagasiTerbuka = false;
bool bagasiTerkunci = true;

void setup() {
  Serial.begin(115200);
  esp_task_wdt_delete(NULL);

  // Delay agar daya stabil
  delay(2000);

  // Set posisi default
  engsel.attach(SERVO_ENGSEL);
  engsel.write(180); // posisi tertutup
  delay(1000);
  engsel.detach();

  kunci.attach(SERVO_KUNCI);
  kunci.write(180); // posisi terkunci
  delay(1000);
  kunci.detach();

  // Start WiFi
  WiFi.softAP(ssid, password);
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  // Routing
  server.on("/", []() {
    String status = "Tertutup & Terkunci";
    if (bagasiTerbuka && !bagasiTerkunci) status = "Terbuka";
    else if (!bagasiTerbuka && bagasiTerkunci == false) status = "Tertutup tapi tidak terkunci";

    String html = R"rawliteral(
      <html><head><meta name="viewport" content="width=device-width, initial-scale=1">
      <style>button { width: 150px; height: 60px; font-size: 20px; margin: 10px; }</style>
      </head><body style="text-align:center;">
      <h2>Kendali Servo Bagasi</h2>
    )rawliteral";
    html += "<p>Status: <b>" + status + "</b></p>";
    html += R"rawliteral(
      <button onclick="fetch('/buka')">BUKA BAGASI</button>
      <button onclick="fetch('/tutup')">TUTUP BAGASI</button>
      </body></html>
    )rawliteral";
    server.send(200, "text/html", html);
  });

  server.on("/buka", []() {
    bukaBagasi();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/tutup", []() {
    tutupBagasi();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}

// Fungsi buka tutup
void bukaBagasi() {
  Serial.println("Membuka bagasi...");
  kunci.attach(SERVO_KUNCI);
  kunci.write(90);  // buka kunci
  delay(2000);
  kunci.detach();

  engsel.attach(SERVO_ENGSEL);
  engsel.write(90);  // buka engsel
  delay(2000);
  engsel.detach();

  bagasiTerbuka = true;
  bagasiTerkunci = false;
}

void tutupBagasi() {
  Serial.println("Menutup bagasi...");
  engsel.attach(SERVO_ENGSEL);
  engsel.write(180);  // tutup engsel
  delay(3000);
  engsel.detach();

  kunci.attach(SERVO_KUNCI);
  kunci.write(180);  // kunci
  delay(2000);
  kunci.detach();

  bagasiTerbuka = false;
  bagasiTerkunci = true;
}
