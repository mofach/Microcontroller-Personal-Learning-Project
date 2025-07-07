#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// WiFi AP
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

// Pin servo
#define SERVO_ENGSEL 16
#define SERVO_KUNCI 17

WebServer server(80);
bool blokirMaju = false;
int kecepatanMotor = 200;  // Default speed (0-255)

Servo engsel, kunci;
bool bagasiTerbuka = false;
bool bagasiTerkunci = true;

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
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

void mundur() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
  blokirMaju = false; // reset blokir
}

void kanan() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
  blokirMaju = false;
}

void kiri() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, kecepatanMotor);
  ledcWrite(ENB, kecepatanMotor);
}

// Baca jarak ultrasonik
long bacaJarak() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long durasi = pulseIn(ECHO, HIGH, 30000); // timeout 30ms
  return durasi * 0.034 / 2;
}

// Halaman Web UI dengan slider kecepatan dan kontrol bagasi
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RC Control</title>
  <style>
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
    }
    button:active {
      background-color: #45a049;
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
  </style>
</head>
<body>
  <div class="container">
    <h2>Kendali Mobil RC</h2>
    
    <div class="speed-control">
      <h3>Kecepatan: <span id="speedValue">200</span>/255</h3>
      <input type="range" id="speedSlider" min="0" max="255" value="200" oninput="updateSpeed()">
    </div>
    
    <div class="control-panel">
      <button onmousedown="sendCommand('/maju')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/maju')" ontouchend="sendCommand('/stop')">MAJU</button><br><br>
      
      <button onmousedown="sendCommand('/kiri')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kiri')" ontouchend="sendCommand('/stop')">KIRI</button>
      
      <button onmousedown="sendCommand('/kanan')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/kanan')" ontouchend="sendCommand('/stop')">KANAN</button><br><br>
      
      <button onmousedown="sendCommand('/mundur')" onmouseup="sendCommand('/stop')"
              ontouchstart="sendCommand('/mundur')" ontouchend="sendCommand('/stop')">MUNDUR</button>
    </div>

    <h2>Kontrol Bagasi</h2>
    <button onclick="sendCommand('/buka')">BUKA BAGASI</button>
    <button onclick="sendCommand('/tutup')">TUTUP BAGASI</button>
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

void setup() {
  Serial.begin(115200);

  // Setup pin motor
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Setup pin sensor & buzzer
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);

  // Setup servo
  engsel.attach(SERVO_ENGSEL);
  engsel.write(180); // posisi tertutup
  delay(1000);
  
  kunci.attach(SERVO_KUNCI);
  kunci.write(180); // posisi terkunci
  delay(1000);

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
  server.on("/setSpeed", []() {
    if (server.hasArg("value")) {
      kecepatanMotor = server.arg("value").toInt();
      server.send(200, "text/plain", "Speed set to " + String(kecepatanMotor));
    } else {
      server.send(400, "text/plain", "Missing speed value");
    }
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

// Fungsi buka tutup bagasi
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
