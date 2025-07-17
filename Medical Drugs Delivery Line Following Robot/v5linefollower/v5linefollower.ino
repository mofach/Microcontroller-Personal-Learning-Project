#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ========== KONFIGURASI PIN ==========
// Motor kanan
#define ENA 13
#define IN1 32
#define IN2 33

// Motor kiri
#define ENB 14
#define IN3 25
#define IN4 26

// Sensor
#define TRIG_PIN 27
#define ECHO_PIN 4
#define BUZZER_PIN 2
#define IR_KIRI   34
#define IR_TENGAH 39
#define IR_KANAN  35

// Servo
#define SERVO_ENGSEL 16
#define SERVO_KUNCI 17

// ========== VARIABEL GLOBAL ==========
WebServer server(80);
const char* ssid = "MobilRC";
const char* password = "12345678";

bool blokirSementara = false;
bool modeOtomatis = false;
int kecepatanMotor = 200;
int arahTerakhir = 0;

Servo engsel, kunci;
bool bagasiTerbuka = false;
bool bagasiTerkunci = true;
unsigned long waktuBlokir = 0;

// ========== FUNGSI MOTOR ==========
void hentikanMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

void gerakMaju() {
  if (blokirSementara) return;
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

void gerakMundur() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

void belokKanan() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

void belokKiri() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

// ========== FUNGSI SENSOR ==========
long bacaJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH, 30000) * 0.034 / 2;
}

void prosesHalangan() {
  long jarak = bacaJarak();
  if (jarak > 0 && jarak <= 15) {
    digitalWrite(BUZZER_PIN, HIGH);
    blokirSementara = true;
    hentikanMotor();
    waktuBlokir = millis();
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    // Jika sudah 2 deton sejak terakhir terblokir, reset blokir
    if (blokirSementara && (millis() - waktuBlokir > 2000)) {
      blokirSementara = false;
    }
  }
}

// ========== FUNGSI OTOMATIS ==========
void jalankanModeOtomatis() {
  if (blokirSementara) return;

  int nilaiKiri = digitalRead(IR_KIRI);
  int nilaiTengah = digitalRead(IR_TENGAH);
  int nilaiKanan = digitalRead(IR_KANAN);

  if (nilaiTengah == HIGH && nilaiKiri == LOW && nilaiKanan == LOW) {
    gerakMaju();
    arahTerakhir = 0;
  }
  else if (nilaiTengah == HIGH && nilaiKiri == HIGH && nilaiKanan == LOW) {
    belokKiri();
    arahTerakhir = -1;
  }
  else if (nilaiTengah == HIGH && nilaiKiri == LOW && nilaiKanan == HIGH) {
    belokKanan();
    arahTerakhir = 1;
  }
  else if (nilaiTengah == LOW && nilaiKiri == HIGH && nilaiKanan == LOW) {
    belokKiri();
    arahTerakhir = -1;
  }
  else if (nilaiTengah == LOW && nilaiKiri == LOW && nilaiKanan == HIGH) {
    belokKanan();
    arahTerakhir = 1;
  }
  else if (nilaiTengah == HIGH && nilaiKiri == HIGH && nilaiKanan == HIGH) {
    gerakMaju();
    arahTerakhir = 0;
  }
  else {
    if (arahTerakhir == -1) belokKiri();
    else if (arahTerakhir == 1) belokKanan();
    else gerakMaju();
  }
}

// ========== HALAMAN WEB ==========
String halamanWeb() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Kendali Mobil RC</title>
  <style>
    * {
      -webkit-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
    }
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 20px;
      background-color: #f5f5f5;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h2 {
      color: #333;
    }
    .control-panel {
      margin-top: 20px;
    }
    button {
      width: 100px;
      height: 60px;
      font-size: 18px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      background-color: #4CAF50;
      color: white;
      cursor: pointer;
      -webkit-tap-highlight-color: transparent;
    }
    .button-red {
      background-color: #f44336;
    }
    .button-blue {
      background-color: #2196F3;
    }
    button:active {
      filter: brightness(90%);
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
    .status-box {
      padding: 10px;
      border-radius: 5px;
      margin: 10px 0;
      font-weight: bold;
    }
    .aktif {
      background-color: #4CAF50;
      color: white;
    }
    .nonaktif {
      background-color: #f44336;
      color: white;
    }
    .peringatan {
      background-color: #FFC107;
      color: black;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Kendali Mobil RC</h2>
    
    <div class="status-box %status_class%">
      Mode: %status_text% %status_peringatan%
    </div>
    
    <div class="speed-control">
      <h3>Kecepatan: <span id="speedValue">200</span>/255</h3>
      <input type="range" id="speedSlider" min="0" max="255" value="200" oninput="updateSpeed()">
    </div>
    
    <div class="control-panel">
      <button onmousedown="sendCommand('/maju')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/maju')" ontouchend="sendCommand('/stop')">MAJU</button><br><br>
      
      <button onmousedown="sendCommand('/kiri')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kiri')" ontouchend="sendCommand('/stop')">BELOK KIRI</button>
      
      <button onmousedown="sendCommand('/kanan')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kanan')" ontouchend="sendCommand('/stop')">BELOK KANAN</button><br><br>
      
      <button onmousedown="sendCommand('/mundur')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/mundur')" ontouchend="sendCommand('/stop')">MUNDUR</button>
    </div>

    <h2>Kontrol Bagasi</h2>
    <button onclick="sendCommand('/buka')">BUKA BAGASI</button>
    <button onclick="sendCommand('/tutup')">TUTUP BAGASI</button>

    <h2>Mode Otomatis</h2>
    <button class="button-blue" onclick="sendCommand('/otomatis/on')">AKTIFKAN</button>
    <button class="button-red" onclick="sendCommand('/otomatis/off')">NONAKTIFKAN</button>
  </div>

  <script>
    function sendCommand(cmd) {
      fetch(cmd).catch(err => console.log('Error:', err));
    }
    
    function updateSpeed() {
      const speedSlider = document.getElementById("speedSlider");
      const speedValue = document.getElementById("speedValue");
      speedValue.textContent = speedSlider.value;
      fetch('/setSpeed?value=' + speedSlider.value).catch(err => console.log('Error:', err));
    }
  </script>
</body>
</html>
)rawliteral";

  String statusClass = modeOtomatis ? "aktif" : "nonaktif";
  String statusText = modeOtomatis ? "OTOMATIS" : "MANUAL";
  String statusWarning = "";
  
  if (blokirSementara && modeOtomatis) {
    statusClass = "peringatan";
    statusWarning = "(TERHENTI - Ada penghalang)";
  }

  html.replace("%status_class%", statusClass);
  html.replace("%status_text%", statusText);
  html.replace("%status_peringatan%", statusWarning);
  return html;
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  // Inisialisasi PIN
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_KIRI, INPUT); pinMode(IR_TENGAH, INPUT); pinMode(IR_KANAN, INPUT);

  // Inisialisasi Servo
  engsel.attach(SERVO_ENGSEL);
  kunci.attach(SERVO_KUNCI);
  engsel.write(180);
  kunci.write(180);
  delay(1000);

  // PWM Motor
  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);
  hentikanMotor();

  // WiFi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Routing Web Server
  server.on("/", []() { server.send(200, "text/html", halamanWeb()); });
  server.on("/maju", []() { if (!modeOtomatis) gerakMaju(); server.send(200, "text/plain", "OK"); });
  server.on("/mundur", []() { if (!modeOtomatis) gerakMundur(); server.send(200, "text/plain", "OK"); });
  server.on("/kanan", []() { if (!modeOtomatis) belokKanan(); server.send(200, "text/plain", "OK"); });
  server.on("/kiri", []() { if (!modeOtomatis) belokKiri(); server.send(200, "text/plain", "OK"); });
  server.on("/stop", []() { hentikanMotor(); server.send(200, "text/plain", "STOP"); });
  server.on("/setSpeed", []() {
    if (server.hasArg("value")) {
      kecepatanMotor = server.arg("value").toInt();
      server.send(200, "text/plain", "Speed set to " + String(kecepatanMotor));
    }
  });
  server.on("/buka", []() { bukaBagasi(); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/tutup", []() { tutupBagasi(); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/otomatis/on", []() { modeOtomatis = true; server.sendHeader("Location", "/"); server.send(303); });
  server.on("/otomatis/off", []() { modeOtomatis = false; hentikanMotor(); server.sendHeader("Location", "/"); server.send(303); });

  server.begin();
}

// ========== LOOP UTAMA ==========
void loop() {
  server.handleClient();
  prosesHalangan();
  
  if (modeOtomatis && !blokirSementara) {
    jalankanModeOtomatis();
  }
}

// ========== FUNGSI BAGASI ==========
void bukaBagasi() {
  kunci.write(90);
  delay(2000);
  engsel.write(90);
  delay(2000);
  bagasiTerbuka = true;
  bagasiTerkunci = false;
}

void tutupBagasi() {
  engsel.write(180);
  delay(3000);
  kunci.write(180);
  delay(2000);
  bagasiTerbuka = false;
  bagasiTerkunci = true;
}