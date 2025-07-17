#include <WiFi.h>
#include <WebServer.h>

// =====================
// WIFI AP SETUP
// =====================
const char* ssid = "LineFollower-ESP32";
const char* password = "12345678";
WebServer server(80);

// =====================
// VARIABEL STATUS
// =====================
bool modeOtomatis = false;
int tujuan = 0; // 0: tidak ada, 1: ruangan A, 2: ruangan B, 3: ruangan C
int intersectionCount = 0;
unsigned long lastIntersectionTime = 0;
bool atIntersection = false;

// =====================
// KONFIGURASI PIN
// =====================
#define ENA 13
#define IN1 32
#define IN2 33
#define ENB 14
#define IN3 25
#define IN4 26
#define IR_LEFT   34
#define IR_CENTER 39
#define IR_RIGHT  35

const bool HITAM = HIGH;
const bool PUTIH = LOW;
const int SPEED = 67;
int lastDirection = 0;

void setup() {
  Serial.begin(115200);

  // Motor
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Sensor IR
  pinMode(IR_LEFT, INPUT); pinMode(IR_CENTER, INPUT); pinMode(IR_RIGHT, INPUT);

  // Buat WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("AP Started");
  Serial.println(WiFi.softAPIP());

  // =====================
  // ROUTE HANDLER
  // =====================
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", halamanWeb());
  });

  server.on("/tujuanA", HTTP_GET, []() {
    modeOtomatis = true;
    tujuan = 1;
    intersectionCount = 0;
    server.send(200, "text/html", halamanWeb());
    Serial.println("Mulai jalan ke Ruangan A");
  });

  server.on("/tujuanB", HTTP_GET, []() {
    modeOtomatis = true;
    tujuan = 2;
    intersectionCount = 0;
    server.send(200, "text/html", halamanWeb());
    Serial.println("Mulai jalan ke Ruangan B");
  });

  server.on("/tujuanC", HTTP_GET, []() {
    modeOtomatis = true;
    tujuan = 3;
    intersectionCount = 0;
    server.send(200, "text/html", halamanWeb());
    Serial.println("Mulai jalan ke Ruangan C");
  });

  server.on("/off", HTTP_GET, []() {
    modeOtomatis = false;
    tujuan = 0;
    stop();
    server.send(200, "text/html", halamanWeb());
    Serial.println("Robot berhenti");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Alur utama robot
  if (modeOtomatis && tujuan > 0) {
    int kiri = digitalRead(IR_LEFT);
    int tengah = digitalRead(IR_CENTER);
    int kanan = digitalRead(IR_RIGHT);

    // Deteksi persimpangan (ketiga sensor hitam)
    if (kiri == HITAM && tengah == HITAM && kanan == HITAM) {
      if (!atIntersection && millis() - lastIntersectionTime > 1000) { // Debounce
        atIntersection = true;
        lastIntersectionTime = millis();
        intersectionCount++;
        Serial.print("Persimpangan ke-");
        Serial.println(intersectionCount);
        
        // Logika belok sesuai tujuan
        if ((tujuan == 1 && intersectionCount == 1) || 
            (tujuan == 2 && intersectionCount == 2) ||
            (tujuan == 3 && intersectionCount == 3)) {
          Serial.println("Belok kanan di persimpangan ini!");
          pivotKanan();
          delay(1500);
          maju();
          delay(1500);
          stop();
          modeOtomatis = false; // Berhenti setelah sampai tujuan
          tujuan = 0;
          return;
        }
      }
    } else {
      atIntersection = false;
      
      // Line follower biasa
      if (tengah == HITAM) {
        if (kiri == PUTIH && kanan == PUTIH) {
          maju(); lastDirection = 0;
        } else if (kiri == HITAM) {
          pivotKiri(); lastDirection = -1;
        } else if (kanan == HITAM) {
          pivotKanan(); lastDirection = 1;
        }
      } else {
        // Fallback cari garis
        if (lastDirection == -1) pivotKiri();
        else if (lastDirection == 1) pivotKanan();
        else maju();
      }
    }
  }
  
  delay(10);
}

// =====================
// FUNGSI MOTOR
// =====================
void maju() {
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void pivotKiri() {
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void pivotKanan() {
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void stop() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// =====================
// HALAMAN HTML - Minimalis
// =====================
String halamanWeb() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Kontrol Robot</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; }";
  html += "h1 { color: #333; }";
  html += ".btn { display: inline-block; padding: 15px 25px; font-size: 18px; cursor: pointer; ";
  html += "text-align: center; text-decoration: none; outline: none; border: none; ";
  html += "border-radius: 15px; margin: 10px; width: 150px; }";
  html += ".btn-tujuan { background-color: #4CAF50; color: white; }";
  html += ".btn-tujuan:active { background-color: #3e8e41; }";
  html += ".btn-off { background-color: #f44336; color: white; }";
  html += ".btn-off:active { background-color: #d32f2f; }";
  html += "</style></head><body>";
  html += "<h1>Kontrol Robot Line Follower</h1>";
  html += "<a href='/tujuanA' class='btn btn-tujuan'>Ruangan A</a>";
  html += "<a href='/tujuanB' class='btn btn-tujuan'>Ruangan B</a>";
  html += "<a href='/tujuanC' class='btn btn-tujuan'>Ruangan C</a>";
  html += "<br><br>";
  html += "<a href='/off' class='btn btn-off'>STOP</a>";
  html += "</body></html>";
  return html;
}
