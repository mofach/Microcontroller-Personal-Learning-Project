# Medical Drugs Delivery Robot

**Medical Drugs Delivery Robot** adalah robot pengantar obat berbasis ESP32. Robot ini dapat berjalan otomatis mengikuti jalur, dikendalikan manual lewat WiFi, dilengkapi sensor ultrasonic anti tabrak, bagasi servo otomatis, dan autentikasi RFID.

---

## 🎯 Fitur Utama

- **Mode Otomatis:** Jalur navigasi mengikuti line sensor ke target ruangan (A, B, C)
- **Mode Manual:** Kendali manual melalui web dashboard via WiFi AP
- **Anti-Tabrak:** Sensor ultrasonic menghentikan robot otomatis saat ada hambatan
- **Bagasi:** Pintu bagasi otomatis buka/tutup dengan servo, aman dengan RFID
- **RFID:** Autentikasi buka/tutup bagasi via kartu RFID
- **Web UI:** Tampilan kendali modern (kontrol gerakan, mode, bagasi)
- **Buzzer:** Penanda saat sampai tujuan atau mendeteksi hambatan

---

## 📌 Pinout

| Components                | Function        | GPIO ESP32 |
|---------------------------|-----------------|------------|
| Right Motor               | ENA (PWM)       | GPIO 13    |
|                           | IN1             | GPIO 32    |
|                           | IN2             | GPIO 33    |
| Left Motor                | ENB (PWM)       | GPIO 14    |
|                           | IN3             | GPIO 25    |
|                           | IN4             | GPIO 26    |
| IR Sensor Center          | Digital Input   | GPIO 39    |
| IR Sensor Right           | Digital Input   | GPIO 35    |
| IR Sensor Left            | Digital Input   | GPIO 34    |
| Ultrasonic Sensor HC-SR04 | TRIG            | GPIO 27    |
|                           | ECHO            | GPIO 4     |
| Servo SG90-> Hinge        | PWM             | GPIO 16    |
| Servo SG90-> Lock         | PWM             | GPIO 17    |
| Active Buzzer 3 Pin       | PWM / I/O       | GPIO 2     |
| RFID RC522                | SDA / SS        | GPIO 5     |
|                           | SCK             | GPIO 18    |
|                           | MOSI            | GPIO 23    |
|                           | MISO            | GPIO 19    |
|                           | RST             | GPIO 22    |
|                           | IRQ             | GPIO 21    |

---

## ⚙️ Cara Menggunakan

1. **Nyalakan robot:** ESP32 akan membuat WiFi Access Point `MobilRC-ESP32` (password: `12345678`).
2. **Hubungkan HP/Laptop:** Sambungkan ke WiFi robot.
3. **Akses Kendali:** Buka `http://192.168.4.1` pada browser.
4. **Pilih Mode:** Manual (kontrol penuh) atau Otomatis (pilih ruangan A/B/C).
5. **Gunakan Bagasi:** Buka/tutup bagasi via tombol di web atau scan kartu RFID.
6. **Keamanan:** Sensor ultrasonic otomatis menghentikan robot jika ada halangan.

---

## ✅ Spesifikasi Singkat

- **Board:** ESP32 WiFi
- **Motor:** Motor DC dengan driver H-Bridge
- **Sensor:** Line Follower 3 titik, Ultrasonic, RFID MFRC522
- **Aktuator:** 2 Servo (Engsel & Kunci), Buzzer
- **Kontrol:** WebServer HTML/CSS/JS responsive
- **Pengembangan:** Arduino IDE

---

**© 2025 Medical Drugs Delivery Robot**  

**Made for Medical Drug Delivery Automation**
