#include <WiFi.h>
#include <WebServer.h>

#define IR_LEFT 34
#define IR_MID 35
#define IR_RIGHT 32

#define ENA 13
#define IN1 25
#define IN2 26
#define ENB 14
#define IN3 27
#define IN4 33

#define BUZZER 2

const char* ssid = "RC-Auto";
const char* password = "12345678";
WebServer server(80);

String mode = "OFF";  // OFF, A, B, C
int simpangCount = 0;
bool stopOnThis = false;
bool persimpanganTerlewati = false;

void setupMotor() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
}

void setupIR() {
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_MID, INPUT);
  pinMode(IR_RIGHT, INPUT);
}

void setupBuzzer() {
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerBeep(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER, HIGH); delay(150);
    digitalWrite(BUZZER, LOW); delay(150);
  }
}

void buzzerOn() {
  digitalWrite(BUZZER, HIGH);
}

void buzzerOff() {
  digitalWrite(BUZZER, LOW);
}

void motorMaju() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 70); analogWrite(ENB, 70);
}

void motorStop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

void motorBelokKanan() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 70); analogWrite(ENB, 70);
}

void handleRoot() {
  String html = "<html><head><title>RC Mode</title></head><body><h1>Pilih Mode Tujuan</h1>";
  html += "<button onclick=\"location.href='/a'\">Ruang A</button> ";
  html += "<button onclick=\"location.href='/b'\">Ruang B</button> ";
  html += "<button onclick=\"location.href='/c'\">Ruang C</button> ";
  html += "<button onclick=\"location.href='/off'\">OFF</button> ";
  html += "<p>Mode Sekarang: <b>" + mode + "</b></p></body></html>";
  server.send(200, "text/html", html);
}

void handleMode(String m) {
  mode = m;
  simpangCount = 0;
  persimpanganTerlewati = false;
  stopOnThis = false;
  motorStop();
  buzzerOff();
  handleRoot();
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/a", [](){ handleMode("A"); });
  server.on("/b", [](){ handleMode("B"); });
  server.on("/c", [](){ handleMode("C"); });
  server.on("/off", [](){ handleMode("OFF"); });
  server.begin();
}

void setupWiFi() {
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP Ready: " + WiFi.softAPIP().toString());
}

bool isPersimpangan() {
  return digitalRead(IR_LEFT) == LOW && digitalRead(IR_MID) == LOW && digitalRead(IR_RIGHT) == LOW;
}

void loopOtomatis() {
  server.handleClient();

  if (mode == "OFF") {
    motorStop();
    buzzerOff();
    return;
  }

  if (isPersimpangan()) {
    delay(200);  // debounce
    if (!persimpanganTerlewati) {
      simpangCount++;
      persimpanganTerlewati = true;

      if ((mode == "A" && simpangCount == 1) ||
          (mode == "B" && simpangCount == 2) ||
          (mode == "C" && simpangCount == 3)) {
        stopOnThis = true;
        buzzerOn();
      } else {
        buzzerBeep(2);
      }
    }
  } else {
    persimpanganTerlewati = false;
    buzzerOff();
  }

  if (stopOnThis) {
    motorBelokKanan();
    delay(600);  // belok kanan
    motorStop();
    return;
  }

  // Normal line following logic (IR tengah hitam = maju)
  int L = digitalRead(IR_LEFT);
  int M = digitalRead(IR_MID);
  int R = digitalRead(IR_RIGHT);

  if (M == LOW) {
    motorMaju();
  } else if (L == LOW) {
    // belok kiri
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (R == LOW) {
    // belok kanan
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else {
    motorStop(); // semua putih
  }
}

void setup() {
  Serial.begin(115200);
  setupMotor();
  setupIR();
  setupBuzzer();
  setupWiFi();
  setupWebServer();
}

void loop() {
  loopOtomatis();
}
