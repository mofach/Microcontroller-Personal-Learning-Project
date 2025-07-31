#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// =====================
// WIFI AP SETUP
// =====================
const char* ssid = "MobilRC-ESP32";
const char* password = "12345678";
WebServer server(80);

// =====================
// PIN DEFINITIONS
// =====================
// Motor kanan
#define ENA 13
#define IN1 32
#define IN2 33

// Motor kiri
#define ENB 14
#define IN3 25
#define IN4 26

// Sensor line follower
#define IR_LEFT   34
#define IR_CENTER 39
#define IR_RIGHT  35

// Sensor ultrasonic
#define TRIG 27
#define ECHO 4

// Buzzer
#define BUZZER_PIN 2

// Servo - Hanya satu servo di pin 16
#define SERVO_ENGSEL 16

// RFID
#define RST_PIN 22
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// =====================
// GLOBAL VARIABLES
// =====================
Servo engsel; // Hanya satu servo
bool bagasiTerbuka = false;
bool blokirMaju = false;
int kecepatanMotor = 70;

// Mode otomatis variables
bool modeOtomatis = false;
bool modeManual = true;
int targetRuangan = 0;
int persimpanganTerlewati = 0;
bool sedangDiPersimpangan = false;
bool buzzerAktif = false;
bool robotBerhenti = false;
bool modePulang = false;
unsigned long waktuBuzzer = 0;

// PERBAIKAN: Tambahan untuk optimasi
unsigned long lastSensorRead = 0;
unsigned long lastWebHandle = 0;
unsigned long lastUltrasonicRead = 0;
bool servoInisialisasi = false;

const bool HITAM = HIGH;
const bool PUTIH = LOW;
const int SPEED = 70;
int lastDirection = 0;

// =====================
// MOTOR CONTROL FUNCTIONS
// =====================
void stopMotor() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void maju() {
  if (blokirMaju && modeManual) return;
  analogWrite(ENA, kecepatanMotor);
  analogWrite(ENB, kecepatanMotor);
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
}

void mundur() {
  analogWrite(ENA, kecepatanMotor);
  analogWrite(ENB, kecepatanMotor);
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
  blokirMaju = false;
}

void pivotKiri() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
}

void pivotKanan() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
}

void kanan() {
  pivotKanan();
  blokirMaju = false;
}

void kiri() {
  pivotKiri();
  blokirMaju = false;
}

void putarBalik() {
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED);
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
  delay(1000);
  stopMotor();
}

// =====================
// SENSOR FUNCTIONS - OPTIMIZED
// =====================
long bacaJarak() {
  // PERBAIKAN: Tambah timeout dan error handling
  digitalWrite(TRIG, LOW); 
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  
  // Timeout 30ms untuk mencegah hang
  long durasi = pulseIn(ECHO, HIGH, 30000);
  if (durasi == 0) return 999; // Error atau terlalu jauh
  
  return durasi * 0.034 / 2;
}

// =====================
// BAGASI FUNCTIONS - OPTIMIZED
// =====================
void bukaBagasi() {
  Serial.println("Membuka bagasi...");
  
  // PERBAIKAN: Gerakan servo yang lebih lebar 0-180 derajat
  if (!engsel.attached()) engsel.attach(SERVO_ENGSEL);
  
  // Gerakan perlahan untuk memastikan servo sampai posisi
  for (int pos = 180; pos >= 0; pos -= 5) {
    engsel.write(pos);
    delay(50);
    yield();
  }
  
  delay(500); // Tunggu sampai posisi final
  engsel.detach(); // PENTING: Detach setelah selesai

  bagasiTerbuka = true;
  Serial.println("Bagasi terbuka");
}

void tutupBagasi() {
  Serial.println("Menutup bagasi...");
  
  if (!engsel.attached()) engsel.attach(SERVO_ENGSEL);
  
  // Gerakan perlahan untuk memastikan servo sampai posisi
  for (int pos = 0; pos <= 180; pos += 5) {
    engsel.write(pos);
    delay(50);
    yield();
  }
  
  delay(500); // Tunggu sampai posisi final
  engsel.detach(); // PENTING: Detach setelah selesai

  bagasiTerbuka = false;
  Serial.println("Bagasi tertutup");
}

// =====================
// BUZZER FUNCTIONS - OPTIMIZED
// =====================
void beepPulang() {
  // PERBAIKAN: Non-blocking buzzer
  digitalWrite(BUZZER_PIN, HIGH); 
  delay(500); // Kurangi delay
  digitalWrite(BUZZER_PIN, LOW);  
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH); 
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);  
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH); 
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

// =====================
// WEB SERVER HANDLERS - OPTIMIZED
// =====================
void handleRoot() {
  // PERBAIKAN: HTML diperkecil dan dioptimasi + No select/copy
  String html = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Kontrol Robot</title>");
  html += F("<style>body{font-family:Arial;text-align:center;margin:0;padding:20px;background:#f5f5f5;");
  html += F("-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;");
  html += F("-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}");
  html += F(".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}");
  html += F(".mode-btn{padding:10px 20px;font-size:16px;margin:5px;border:none;border-radius:5px;cursor:pointer}");
  html += F(".manual-btn{background:#4CAF50;color:white}.auto-btn{background:#2196F3;color:white}");
  html += F(".control-btn{width:100px;height:60px;font-size:18px;margin:5px;border:none;border-radius:5px;background:#4CAF50;color:white;cursor:pointer}");
  html += F(".bagasi-btn{padding:10px 20px;font-size:16px;margin:5px;border:none;border-radius:5px;background:#FF9800;color:white;cursor:pointer}");
  html += F(".auto-options{display:none}*{-webkit-touch-callout:none;-webkit-user-select:none}</style></head><body>");
  
  html += F("<div class='container'><h2>Kendali Robot</h2>");
  html += F("<div><button id='manualBtn' class='mode-btn manual-btn' onclick='switchMode(\"manual\")'>MANUAL</button>");
  html += F("<button id='autoBtn' class='mode-btn auto-btn' onclick='switchMode(\"auto\")'>OTOMATIS</button></div>");
  
  html += F("<div id='manualPanel'>");
  html += F("<div><h3>Kecepatan: <span id='speedValue'>200</span>/255</h3>");
  html += F("<input type='range' id='speedSlider' min='0' max='255' value='200' oninput='updateSpeed()'></div>");
  html += F("<button ontouchstart='startCommand(\"/maju\")' ontouchend='stopCommand()' onmousedown='startCommand(\"/maju\")' onmouseup='stopCommand()' class='control-btn'>MAJU</button><br>");
  html += F("<button ontouchstart='startCommand(\"/kiri\")' ontouchend='stopCommand()' onmousedown='startCommand(\"/kiri\")' onmouseup='stopCommand()' class='control-btn'>KIRI</button>");
  html += F("<button ontouchstart='startCommand(\"/kanan\")' ontouchend='stopCommand()' onmousedown='startCommand(\"/kanan\")' onmouseup='stopCommand()' class='control-btn'>KANAN</button><br>");
  html += F("<button ontouchstart='startCommand(\"/mundur\")' ontouchend='stopCommand()' onmousedown='startCommand(\"/mundur\")' onmouseup='stopCommand()' class='control-btn'>MUNDUR</button></div>");
  
  html += F("<div id='autoPanel' class='auto-options'>");
  html += F("<button onclick='sendCommand(\"/ruangan_a\")' class='control-btn' style='background:blue'>RUANGAN A</button>");
  html += F("<button onclick='sendCommand(\"/ruangan_b\")' class='control-btn' style='background:green'>RUANGAN B</button>");
  html += F("<button onclick='sendCommand(\"/ruangan_c\")' class='control-btn' style='background:orange'>RUANGAN C</button><br>");
  html += F("<button onclick='sendCommand(\"/off\")' class='control-btn' style='background:red'>STOP</button>");
  html += F("<button onclick='sendCommand(\"/pulang\")' class='control-btn' style='background:purple'>PULANG</button></div>");
  
  html += F("<div><h3>Kontrol Bagasi</h3>");
  html += F("<button onclick='sendCommand(\"/buka\")' class='bagasi-btn'>BUKA</button>");
  html += F("<button onclick='sendCommand(\"/tutup\")' class='bagasi-btn'>TUTUP</button></div>");
  html += F("<div id='status'></div></div>");
  
  html += F("<script>");
  html += F("let isPressed=false,currentCmd='';");
  html += F("function startCommand(cmd){if(!isPressed){isPressed=true;currentCmd=cmd;sendCommand(cmd);}}");
  html += F("function stopCommand(){if(isPressed){isPressed=false;sendCommand('/stop');currentCmd='';}}");
  html += F("function switchMode(mode){");
  html += F("document.getElementById('manualPanel').style.display=mode==='manual'?'block':'none';");
  html += F("document.getElementById('autoPanel').style.display=mode==='auto'?'block':'none';");
  html += F("sendCommand('/set_mode?value='+mode);}");
  html += F("function sendCommand(cmd){fetch(cmd).catch(err=>console.log('Error:',err));}");
  html += F("function updateSpeed(){const s=document.getElementById('speedSlider');document.getElementById('speedValue').textContent=s.value;sendCommand('/setSpeed?value='+s.value);}");
  html += F("document.addEventListener('contextmenu',e=>e.preventDefault());");
  html += F("document.addEventListener('selectstart',e=>e.preventDefault());");
  html += F("</script></body></html>");
  
  server.send(200, "text/html", html);
}

void handleSetMode() {
  if (server.hasArg("value")) {
    String mode = server.arg("value");
    if (mode == "manual") {
      modeManual = true;
      modeOtomatis = false;
      stopMotor();
    } else if (mode == "auto") {
      modeManual = false;
      modeOtomatis = true;
    }
    server.send(200, "text/plain", "OK");
  }
}

void handleSetSpeed() {
  if (server.hasArg("value")) {
    kecepatanMotor = server.arg("value").toInt();
    server.send(200, "text/plain", "OK");
  }
}

// =====================
// MAIN SETUP - OPTIMIZED
// =====================
void setup() {
  Serial.begin(115200);

  // Initialize motor pins
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotor();

  // Initialize sensor pins
  pinMode(IR_LEFT, INPUT); pinMode(IR_CENTER, INPUT); pinMode(IR_RIGHT, INPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // PERBAIKAN: Servo initialization yang lebih baik
  // Jangan attach servo di setup, attach hanya saat digunakan
  
  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // Start WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("AP Started");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // PERBAIKAN: Handler yang lebih ringan
  server.on("/", handleRoot);
  server.on("/set_mode", handleSetMode);
  server.on("/setSpeed", handleSetSpeed);
  
  // Manual control routes - OPTIMIZED
  server.on("/maju", []() { 
    if (modeManual) maju(); 
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/mundur", []() { 
    if (modeManual) mundur(); 
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/kanan", []() { 
    if (modeManual) kanan(); 
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/kiri", []() { 
    if (modeManual) kiri(); 
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/stop", []() { 
    stopMotor(); 
    server.send(200, "text/plain", "OK"); 
  });
  
  // Automatic control routes - OPTIMIZED
  server.on("/ruangan_a", []() { 
    modeOtomatis = true; 
    modeManual = false;
    targetRuangan = 1; 
    persimpanganTerlewati = 0;
    sedangDiPersimpangan = false;
    buzzerAktif = false;
    robotBerhenti = false;
    modePulang = false;
    digitalWrite(BUZZER_PIN, LOW);
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/ruangan_b", []() { 
    modeOtomatis = true; 
    modeManual = false;
    targetRuangan = 2; 
    persimpanganTerlewati = 0;
    sedangDiPersimpangan = false;
    buzzerAktif = false;
    robotBerhenti = false;
    modePulang = false;
    digitalWrite(BUZZER_PIN, LOW);
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/ruangan_c", []() { 
    modeOtomatis = true; 
    modeManual = false;
    targetRuangan = 3; 
    persimpanganTerlewati = 0;
    sedangDiPersimpangan = false;
    buzzerAktif = false;
    robotBerhenti = false;
    modePulang = false;
    digitalWrite(BUZZER_PIN, LOW);
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/off", []() { 
    modeOtomatis = false; 
    targetRuangan = 0;
    robotBerhenti = false;
    stopMotor(); 
    digitalWrite(BUZZER_PIN, LOW);
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/pulang", []() { 
    if (targetRuangan > 0 && robotBerhenti) {
      modePulang = true;
      robotBerhenti = false;
      persimpanganTerlewati = 0;
      sedangDiPersimpangan = false;
      buzzerAktif = false;
      digitalWrite(BUZZER_PIN, LOW);
      putarBalik();
      server.send(200, "text/plain", "OK"); 
    } else {
      server.send(200, "text/plain", "Belum sampai"); 
    }
  });
  
  // Bagasi routes
  server.on("/buka", []() { 
    bukaBagasi(); 
    server.send(200, "text/plain", "OK"); 
  });
  server.on("/tutup", []() { 
    tutupBagasi(); 
    server.send(200, "text/plain", "OK"); 
  });

  server.begin();
  Serial.println("Setup complete");
}

// =====================
// MAIN LOOP - HEAVILY OPTIMIZED
// =====================
void loop() {
  unsigned long currentTime = millis();
  
  // PERBAIKAN: Handle web client dengan throttling
  if (currentTime - lastWebHandle >= 10) {
    server.handleClient();
    lastWebHandle = currentTime;
    yield(); // Penting untuk multitasking
  }

  // PERBAIKAN: RFID check dengan throttling
  static unsigned long lastRFIDCheck = 0;
  if (currentTime - lastRFIDCheck >= 300) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      Serial.println("RFID detected");
      if (bagasiTerbuka) {
        tutupBagasi();
      } else {
        bukaBagasi();
      }
      mfrc522.PICC_HaltA();
    }
    lastRFIDCheck = currentTime;
  }

  if (modeOtomatis && targetRuangan > 0) {
    // PERBAIKAN: Sensor reading dengan throttling
    if (currentTime - lastSensorRead >= 50) { // 20Hz instead of 100Hz
      int kiri = digitalRead(IR_LEFT);
      int tengah = digitalRead(IR_CENTER);
      int kanan = digitalRead(IR_RIGHT);

      bool persimpanganTerdeteksi = (kiri == HITAM && tengah == HITAM && kanan == HITAM);

      if (robotBerhenti) {
        stopMotor();
        if (!buzzerAktif) {
          buzzerAktif = true;
          digitalWrite(BUZZER_PIN, HIGH);
        }
      } else if (persimpanganTerdeteksi && !sedangDiPersimpangan) {
        persimpanganTerlewati++;
        sedangDiPersimpangan = true;

        bool persimpanganTarget = (!modePulang && 
          ((targetRuangan == 1 && persimpanganTerlewati == 1) ||
           (targetRuangan == 2 && persimpanganTerlewati == 2) ||
           (targetRuangan == 3 && persimpanganTerlewati == 3))) ||
          (modePulang && 
          ((targetRuangan == 1 && persimpanganTerlewati == 1) ||
           (targetRuangan == 2 && persimpanganTerlewati == 2) ||
           (targetRuangan == 3 && persimpanganTerlewati == 3)));

        if (persimpanganTarget && modePulang) {
          robotBerhenti = true;
          stopMotor();
          beepPulang();
        } else if (persimpanganTarget) {
          robotBerhenti = true;
          stopMotor();
          buzzerAktif = true;
          digitalWrite(BUZZER_PIN, HIGH);
        } else {
          buzzerAktif = true;
          digitalWrite(BUZZER_PIN, HIGH);
        }
      }

      if (buzzerAktif && !persimpanganTerdeteksi && !robotBerhenti) {
        buzzerAktif = false;
        digitalWrite(BUZZER_PIN, LOW);
      }

      if (!persimpanganTerdeteksi && sedangDiPersimpangan) {
        sedangDiPersimpangan = false;
      }

      // Line following logic
      if (!robotBerhenti) {
        if (tengah == HITAM && kiri == PUTIH && kanan == PUTIH) {
          maju(); lastDirection = 0;
        }
        else if (tengah == HITAM && kiri == HITAM && kanan == PUTIH) {
          pivotKiri(); lastDirection = -1;
        }
        else if (tengah == HITAM && kiri == PUTIH && kanan == HITAM) {
          pivotKanan(); lastDirection = 1;
        }
        else if (tengah == PUTIH && kiri == HITAM && kanan == PUTIH) {
          pivotKiri(); lastDirection = -1;
        }
        else if (tengah == PUTIH && kiri == PUTIH && kanan == HITAM) {
          pivotKanan(); lastDirection = 1;
        }
        else if (tengah == HITAM && kiri == HITAM && kanan == HITAM) {
          maju(); lastDirection = 0;
        }
        else {
          if (lastDirection == -1) pivotKiri();
          else if (lastDirection == 1) pivotKanan();
          else maju();
        }
      }
      
      lastSensorRead = currentTime;
    }
  } 
  else if (modeManual) {
    // PERBAIKAN: Ultrasonic dengan throttling yang lebih agresif
    if (currentTime - lastUltrasonicRead >= 200) { // 5Hz instead of 100Hz
      long jarak = bacaJarak();
      if (jarak > 0 && jarak <= 15) {
        digitalWrite(BUZZER_PIN, HIGH);
        blokirMaju = true;
        if (digitalRead(IN2) == HIGH && digitalRead(IN4) == HIGH) {
          stopMotor();
        }
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        blokirMaju = false;
      }
      lastUltrasonicRead = currentTime;
    }
  }
  
  // PERBAIKAN: Minimal delay dan yield
  yield();
  delay(5); // Kurangi dari 10 ke 5
}
