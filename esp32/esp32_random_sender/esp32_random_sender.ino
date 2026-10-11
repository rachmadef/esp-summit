/*
  ESP32 Random Telemetry Data Sender
  Proyek: ESP32 Monitoring System (Laragon + Express + Vue 3)
  
  Deskripsi:
  Program ini menghubungkan ESP32 ke jaringan Wi-Fi lokal dan mengirimkan
  data sensor acak (simulasi) ke server backend melalui HTTP POST JSON.
  
  Perhatian:
  - ESP32 dan komputer server HARUS terhubung ke jaringan Wi-Fi yang sama.
  - Jangan gunakan "localhost" pada SERVER_URL. Gunakan IP lokal komputer Anda.
*/

#include <WiFi.h>
#include <HTTPClient.h>

// ================= KONFIGURASI WI-FI =================
const char* WIFI_SSID     = "NAMA_WIFI_ANDA";       // Ganti dengan SSID Wi-Fi Anda
const char* WIFI_PASSWORD = "PASSWORD_WIFI_ANDA";   // Ganti dengan Password Wi-Fi Anda

// ================= KONFIGURASI SERVER =================
// Ganti IP di bawah jika IP komputer Anda berubah (cek dengan 'ipconfig' di terminal)
const char* SERVER_URL    = "http://192.168.108.12:3000/api/data";
const char* DEVICE_ID     = "esp32-node-01";

// Interval pengiriman data (dalam milidetik: 5000 ms = 5 detik)
const unsigned long SEND_INTERVAL_MS = 5000;

// Pin LED internal (opsional untuk indikator kedipan pengiriman)
#if defined(LED_BUILTIN)
  const int LED_PIN = LED_BUILTIN;
#else
  const int LED_PIN = 2; // Default umum untuk banyak board ESP32
#endif

// ================= VARIABEL GLOBAL =================
unsigned long lastSendTime = 0;
unsigned long packetCounter = 0;

// Fungsi pembantu untuk menghasilkan angka desimal acak
float getRandomFloat(float minVal, float maxVal) {
  float randomRatio = (float)random(0, 10000) / 10000.0;
  return minVal + (randomRatio * (maxVal - minVal));
}

// Fungsi koneksi Wi-Fi
void connectWiFi() {
  Serial.println();
  Serial.print("[WiFi] Menghubungkan ke: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("[WiFi] Berhasil terhubung!");
    Serial.print("[WiFi] IP Address ESP32: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Signal (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("[WiFi] Gagal terhubung. Periksa SSID dan Password!");
  }
}

// Fungsi untuk mengirim data JSON ke backend
void sendRandomTelemetry() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Peringatan] Wi-Fi terputus, mencoba menghubungkan kembali...");
    WiFi.reconnect();
    return;
  }

  // Nyalakan LED indikator saat proses pengiriman
  digitalWrite(LED_PIN, HIGH);

  packetCounter++;

  // 1. Generate nilai data acak
  float suhu = getRandomFloat(24.0, 34.0);        // Rentang 24.0 - 34.0 °C
  float kelembaban = getRandomFloat(45.0, 85.0);  // Rentang 45.0 - 85.0 %
  float tegangan = getRandomFloat(3.15, 3.35);    // Rentang 3.15 - 3.35 V
  int sinyalWifi = WiFi.RSSI();

  // Status acak
  const char* statusList[] = {"normal", "optimal", "waspada"};
  const char* status = statusList[random(0, 3)];

  // 2. Susun format JSON string
  // Payload mengikuti spesifikasi: { "device_id": "...", "data": { ... } }
  String jsonPayload = "{";
  jsonPayload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonPayload += "\"data\":{";
  jsonPayload += "\"suhu\":" + String(suhu, 1) + ",";
  jsonPayload += "\"kelembaban\":" + String(kelembaban, 1) + ",";
  jsonPayload += "\"tegangan\":" + String(tegangan, 2) + ",";
  jsonPayload += "\"paket_ke\":" + String(packetCounter) + ",";
  jsonPayload += "\"rssi_dbm\":" + String(sinyalWifi) + ",";
  jsonPayload += "\"status\":\"" + String(status) + "\"";
  jsonPayload += "}";
  jsonPayload += "}";

  Serial.println("--------------------------------------------------");
  Serial.print("[HTTP] Mengirim data ke: ");
  Serial.println(SERVER_URL);
  Serial.print("[HTTP] Payload: ");
  Serial.println(jsonPayload);

  // 3. Inisialisasi HTTPClient
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(4000); // Batas waktu respons 4 detik

  // 4. Kirim request POST
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.print("[HTTP] Kode Respons: ");
    Serial.println(httpResponseCode);

    String responseBody = http.getString();
    Serial.print("[HTTP] Respons Server: ");
    Serial.println(responseBody);
  } else {
    Serial.print("[HTTP] Pengiriman Gagal! Error: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
    Serial.println("[Tips] Pastikan server backend berjalan dan firewall mengizinkan port 3000.");
  }

  http.end();

  // Matikan LED indikator
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println("==================================================");
  Serial.println("   ESP32 Monitoring System - Random Sender        ");
  Serial.println("==================================================");

  // Inisialisasi seed angka acak
  randomSeed(analogRead(0) + micros());

  connectWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  // Kirim data setiap interval yang ditentukan
  if (currentMillis - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = currentMillis;
    sendRandomTelemetry();
  }
}
