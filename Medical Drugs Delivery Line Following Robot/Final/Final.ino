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

// Servo
#define SERVO_ENGSEL 16
#define SERVO_KUNCI 17

// RFID
#define RST_PIN 22
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// =====================
// GLOBAL VARIABLES
// =====================
Servo engsel, kunci;
bool bagasiTerbuka = false;
bool bagasiTerkunci = true;
bool blokirMaju = false;
int kecepatanMotor = 200;

// Mode otomatis variables
bool modeOtomatis = false;
bool modeManual = true;
int targetRuangan = 0; // 0=off, 1=A, 2=B, 3=C
int persimpanganTerlewati = 0;
bool sedangDiPersimpangan = false;
bool buzzerAktif = false;
bool robotBerhenti = false;
bool modePulang = false;
unsigned long waktuBuzzer = 0;

const bool HITAM = HIGH;
const bool PUTIH = LOW;
const int SPEED = 70;
int lastDirection = 0;

// =====================
// MOTOR CONTROL FUNCTIONS
// =====================
void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void maju() {
  if (blokirMaju && modeManual) return;
  analogWrite(ENA, kecepatanMotor);
  analogWrite(ENB, kecepatanMotor);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void mundur() {
  analogWrite(ENA, kecepatanMotor);
  analogWrite(ENB, kecepatanMotor);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  blokirMaju = false;
}

void pivotKiri() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void pivotKanan() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
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
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  delay(1000);
  stopMotor();
}

// =====================
// SENSOR FUNCTIONS
// =====================
long bacaJarak() {
  digitalWrite(TRIG, LOW); 
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long durasi = pulseIn(ECHO, HIGH, 30000);
  return durasi * 0.034 / 2;
}

// =====================
// BAGASI FUNCTIONS
// =====================
void bukaBagasi() {
  Serial.println("Membuka bagasi...");
  kunci.attach(SERVO_KUNCI);
  kunci.write(90);  // buka kunci
  delay(2000); // Tunggu lebih lama

  engsel.attach(SERVO_ENGSEL);
  engsel.write(90);  // buka engsel
  delay(2000); // Tunggu lebih lama

  bagasiTerbuka = true;
  bagasiTerkunci = false;
}

void tutupBagasi() {
  Serial.println("Menutup bagasi...");
  engsel.attach(SERVO_ENGSEL);
  engsel.write(180);  // tutup engsel
  delay(3000); // Tunggu lebih lama

  kunci.attach(SERVO_KUNCI);
  kunci.write(180);  // kunci
  delay(2000); // Tunggu lebih lama

  bagasiTerbuka = false;
  bagasiTerkunci = true;
}

// =====================
// BUZZER FUNCTIONS
// =====================
void beepPulang() {
  digitalWrite(BUZZER_PIN, HIGH); delay(1000);
  digitalWrite(BUZZER_PIN, LOW);  delay(300);
  digitalWrite(BUZZER_PIN, HIGH); delay(200);
  digitalWrite(BUZZER_PIN, LOW);  delay(200);
  digitalWrite(BUZZER_PIN, HIGH); delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

// =====================
// WEB SERVER HANDLERS
// =====================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Kontrol Mobil RC</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background-color: #f5f5f5;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h2 {
      color: #333;
      margin-bottom: 20px;
    }
    .mode-switch {
      margin: 20px 0;
      padding: 10px;
      background-color: #eee;
      border-radius: 5px;
    }
    .mode-btn {
      padding: 10px 20px;
      font-size: 16px;
      margin: 0 5px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    .manual-btn {
      background-color: #4CAF50;
      color: white;
    }
    .auto-btn {
      background-color: #2196F3;
      color: white;
    }
    .active {
      opacity: 0.8;
      transform: scale(0.95);
    }
    .control-panel {
      margin: 20px 0;
    }
    .control-btn {
      width: 100px;
      height: 60px;
      font-size: 18px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      background-color: #4CAF50;
      color: white;
      cursor: pointer;
    }
    .speed-control {
      margin: 20px 0;
    }
    input[type=range] {
      width: 80%;
      height: 15px;
      -webkit-appearance: none;
      background: #ddd;
      border-radius: 5px;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
    }
    #speedValue {
      font-weight: bold;
      color: #4CAF50;
    }
    .auto-options {
      margin: 20px 0;
      display: none;
    }
    .bagasi-btn {
      padding: 10px 20px;
      font-size: 16px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      background-color: #FF9800;
      color: white;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Kendali Mobil RC</h2>
    
    <div class="mode-switch">
      <button id="manualBtn" class="mode-btn manual-btn active" onclick="switchMode('manual')">MANUAL</button>
      <button id="autoBtn" class="mode-btn auto-btn" onclick="switchMode('auto')">OTOMATIS</button>
    </div>

    <!-- Manual Control Panel -->
    <div id="manualPanel" class="control-panel">
      <div class="speed-control">
        <h3>Kecepatan: <span id="speedValue">200</span>/255</h3>
        <input type="range" id="speedSlider" min="0" max="255" value="200" oninput="updateSpeed()">
      </div>
      
      <button onmousedown="sendCommand('/maju')" onmouseup="sendCommand('/stop')" 
              ontouchstart="sendCommand('/maju')" ontouchend="sendCommand('/stop')" 
              class="control-btn">MAJU</button><br>
      <button onmousedown="sendCommand('/kiri')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kiri')" ontouchend="sendCommand('/stop')"
              class="control-btn">KIRI</button>
      <button onmousedown="sendCommand('/kanan')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kanan')" ontouchend="sendCommand('/stop')"
              class="control-btn">KANAN</button><br>
      <button onmousedown="sendCommand('/mundur')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/mundur')" ontouchend="sendCommand('/stop')"
              class="control-btn">MUNDUR</button>
    </div>

    <!-- Automatic Control Panel -->
    <div id="autoPanel" class="auto-options">
      <button onclick="sendCommand('/ruangan_a')" class="control-btn" style="background-color:blue">RUANGAN A</button>
      <button onclick="sendCommand('/ruangan_b')" class="control-btn" style="background-color:green">RUANGAN B</button>
      <button onclick="sendCommand('/ruangan_c')" class="control-btn" style="background-color:orange">RUANGAN C</button><br>
      <button onclick="sendCommand('/off')" class="control-btn" style="background-color:red">STOP</button>
      <button onclick="sendCommand('/pulang')" class="control-btn" style="background-color:purple">PULANG</button>
    </div>

    <!-- Bagasi Controls -->
    <div style="margin-top:30px">
      <h3>Kontrol Bagasi</h3>
      <button onclick="sendCommand('/buka')" class="bagasi-btn">BUKA BAGASI</button>
      <button onclick="sendCommand('/tutup')" class="bagasi-btn">TUTUP BAGASI</button>
    </div>

    <div id="status" style="margin-top:20px;padding:10px;background-color:#f0f0f0;border-radius:5px;"></div>
  </div>

  <script>
    let currentMode = 'manual';
    
    function switchMode(mode) {
      currentMode = mode;
      document.getElementById('manualBtn').classList.remove('active');
      document.getElementById('autoBtn').classList.remove('active');
      document.getElementById(mode + 'Btn').classList.add('active');
      
      document.getElementById('manualPanel').style.display = 
        mode === 'manual' ? 'block' : 'none';
      document.getElementById('autoPanel').style.display = 
        mode === 'auto' ? 'block' : 'none';
      
      sendCommand('/set_mode?value=' + mode);
      updateStatus('Mode ' + mode.toUpperCase() + ' aktif');
    }

    function sendCommand(cmd) {
      fetch(cmd).catch(err => console.log('Error:', err));
      updateStatus('Perintah dikirim: ' + cmd);
    }
    
    function updateSpeed() {
      const speedSlider = document.getElementById("speedSlider");
      const speedValue = document.getElementById("speedValue");
      speedValue.textContent = speedSlider.value;
      sendCommand('/setSpeed?value=' + speedSlider.value);
    }

    function updateStatus(message) {
      document.getElementById('status').innerHTML = message;
    }

    // Initialize
    window.onload = function() {
      switchMode('manual');
    };
  </script>
</body>
</html>
)rawliteral";
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
    server.send(200, "text/plain", "Mode set to " + mode);
  }
}

void handleSetSpeed() {
  if (server.hasArg("value")) {
    kecepatanMotor = server.arg("value").toInt();
    server.send(200, "text/plain", "Speed set to " + String(kecepatanMotor));
  }
}

// =====================
// MAIN SETUP
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

  // Initialize servos
  engsel.attach(SERVO_ENGSEL);
  engsel.write(180); // posisi tertutup
  delay(1000);
  
  kunci.attach(SERVO_KUNCI);
  kunci.write(180); // posisi terkunci
  delay(1000);

  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);

  // Start WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("AP Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/set_mode", handleSetMode);
  server.on("/setSpeed", handleSetSpeed);
  
  // Manual control routes
  server.on("/maju", []() { if (modeManual) maju(); server.send(200, "text/plain", "OK"); });
  server.on("/mundur", []() { if (modeManual) mundur(); server.send(200, "text/plain", "OK"); });
  server.on("/kanan", []() { if (modeManual) kanan(); server.send(200, "text/plain", "OK"); });
  server.on("/kiri", []() { if (modeManual) kiri(); server.send(200, "text/plain", "OK"); });
  server.on("/stop", []() { stopMotor(); server.send(200, "text/plain", "STOP"); });
  
  // Automatic control routes
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
    server.send(200, "text/plain", "Target: Ruangan A"); 
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
    server.send(200, "text/plain", "Target: Ruangan B"); 
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
    server.send(200, "text/plain", "Target: Ruangan C"); 
  });
  server.on("/off", []() { 
    modeOtomatis = false; 
    targetRuangan = 0;
    robotBerhenti = false;
    stopMotor(); 
    digitalWrite(BUZZER_PIN, LOW);
    server.send(200, "text/plain", "Mode OFF"); 
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
      server.send(200, "text/plain", "Mode PULANG"); 
    } else {
      server.send(200, "text/plain", "Belum sampai tujuan"); 
    }
  });
  
  // Bagasi routes
  server.on("/buka", []() { bukaBagasi(); server.send(200, "text/plain", "Bagasi dibuka"); });
  server.on("/tutup", []() { tutupBagasi(); server.send(200, "text/plain", "Bagasi ditutup"); });

  server.begin();
}

// =====================
// MAIN LOOP
// =====================
void loop() {
  server.handleClient();

  // Handle RFID in both modes
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("RFID card detected!");
    if (bagasiTerbuka) {
      tutupBagasi();
    } else {
      bukaBagasi();
    }
    mfrc522.PICC_HaltA();
    delay(300);
  }

  if (modeOtomatis && targetRuangan > 0) {
    // Automatic mode logic
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
        waktuBuzzer = millis();
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
  } 
  else if (modeManual) {
    // Manual mode obstacle detection
    long jarak = bacaJarak();
    if (jarak > 0 && jarak <= 15) {
      digitalWrite(BUZZER_PIN, HIGH);
      blokirMaju = true;
      if (digitalRead(IN2) == HIGH && digitalRead(IN4) == HIGH) {
        stopMotor();
      }
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  
  delay(10);
}