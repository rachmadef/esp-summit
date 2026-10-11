/*
  ESP32 Random Text & Telemetry Data Sender (Setiap 3 Detik)
  Proyek: ESP32 Monitoring System (Laragon + Express + Vue 3)
  
  Deskripsi:
  Program ini menghubungkan ESP32 ke jaringan Wi-Fi lokal dan mengirimkan
  teks acak serta kode acak ke backend setiap 3 detik sekali via HTTP POST JSON.
*/

#include <WiFi.h>
#include <HTTPClient.h>

// ================= KONFIGURASI WI-FI =================
const char* WIFI_SSID     = "NAMA_WIFI_ANDA";       // Ganti dengan SSID Wi-Fi Anda
const char* WIFI_PASSWORD = "PASSWORD_WIFI_ANDA";   // Ganti dengan Password Wi-Fi Anda

// ================= KONFIGURASI SERVER =================
// Ganti IP jika IP komputer Anda berubah (cek dengan 'ipconfig' di terminal)
const char* SERVER_URL    = "http://192.168.108.12:3000/api/data";
const char* DEVICE_ID     = "esp32-node-01";

// Interval pengiriman data: 3000 milidetik = 3 detik
const unsigned long SEND_INTERVAL_MS = 3000;

// Pin LED internal (indikator saat proses kirim)
#if defined(LED_BUILTIN)
  const int LED_PIN = LED_BUILTIN;
#else
  const int LED_PIN = 2; // Default pin LED di banyak board ESP32
#endif

// ================= VARIABEL GLOBAL =================
unsigned long lastSendTime = 0;
unsigned long packetCounter = 0;

// Daftar teks / pesan acak
const char* daftarPesan[] = {
  "Sistem beroperasi normal",
  "Kondisi lingkungan aman",
  "Pendeteksian aktivitas zona 1",
  "Sensor membaca getaran ringan",
  "Pintu akses utama terkunci",
  "Temperatur ruang optimal",
  "Koneksi jaringan Wi-Fi lancar",
  "Pembaruan status perangkat siap",
  "Peringatan pergerakan terdeteksi",
  "Baterai dalam kondisi prima"
};
const int totalPesan = sizeof(daftarPesan) / sizeof(daftarPesan[0]);

// Fungsi membuat kode string teks acak (contoh: "TXT-7K2M")
String generateRandomCode(int panjang) {
  const char karakter[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String hasil = "TXT-";
  for (int i = 0; i < panjang; i++) {
    hasil += karakter[random(0, sizeof(karakter) - 1)];
  }
  return hasil;
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
    Serial.print("[WiFi] IP ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("[WiFi] Gagal terhubung! Periksa SSID & Password.");
  }
}

// Fungsi mengirim teks random ke backend setiap 3 detik
void sendRandomText() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Terputus, mencoba menghubungkan ulang...");
    WiFi.reconnect();
    return;
  }

  // Nyalakan LED sesaat saat mengirim
  digitalWrite(LED_PIN, HIGH);

  packetCounter++;

  // 1. Ambil teks pesan acak dan kode teks acak
  int indexAcak = random(0, totalPesan);
  String teksPesan = daftarPesan[indexAcak];
  String kodeAcak = generateRandomCode(4);
  int nilaiAcak = random(10, 100);

  // 2. Susun format JSON string
  String jsonPayload = "{";
  jsonPayload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonPayload += "\"data\":{";
  jsonPayload += "\"pesan_teks\":\"" + teksPesan + "\",";
  jsonPayload += "\"kode_acak\":\"" + kodeAcak + "\",";
  jsonPayload += "\"nilai\":" + String(nilaiAcak) + ",";
  jsonPayload += "\"paket_ke\":" + String(packetCounter) + ",";
  jsonPayload += "\"rssi_dbm\":" + String(WiFi.RSSI());
  jsonPayload += "}";
  jsonPayload += "}";

  Serial.println("--------------------------------------------------");
  Serial.print("[HTTP POST 3s] Paket ke #");
  Serial.println(packetCounter);
  Serial.print("[Payload] ");
  Serial.println(jsonPayload);

  // 3. Inisialisasi HTTPClient
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2500); // Batas respons 2.5 detik

  // 4. Kirim data
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.print("[Sukses] Kode HTTP: ");
    Serial.println(httpResponseCode);
    String responseBody = http.getString();
    Serial.print("[Server] ");
    Serial.println(responseBody);
  } else {
    Serial.print("[Error] Pengiriman gagal: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }

  http.end();

  // Matikan LED
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Seed acak berdasarkan noise pin analog
  randomSeed(analogRead(0) + micros());

  Serial.println();
  Serial.println("==================================================");
  Serial.println("   ESP32 Auto Sender (Setiap 3 Detik Teks Acak)   ");
  Serial.println("==================================================");

  connectWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  // Logic: kirim data persis setiap 3000 ms (3 detik)
  if (currentMillis - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = currentMillis;
    sendRandomText();
  }
}
