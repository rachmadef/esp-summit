# ESP32 Monitoring System

Panduan proyek untuk AI coding agent yang membantu membangun sistem pengiriman data dari ESP32 ke database lokal Laragon, dengan backend Node.js + Express.js dan frontend Vue.js 3.

## 1. Tujuan Proyek

Membangun aplikasi lokal yang dapat:

- Menerima data dari ESP32 melalui HTTP REST API dalam format JSON.
- Memvalidasi dan menyimpan data ke MySQL/MariaDB yang dijalankan melalui Laragon.
- Menampilkan data melalui dashboard web Vue.js 3.
- Menampilkan riwayat data dan status perangkat.
- Dikembangkan secara bertahap tanpa mengasumsikan jenis data perangkat yang belum ditentukan.

**Catatan:** Jenis data ESP32 yang sebenarnya belum ditentukan. Gunakan payload contoh hanya untuk pengembangan awal dan buat struktur yang mudah disesuaikan.

## 2. Stack Teknologi

### Backend
- Node.js (versi LTS yang kompatibel)
- Express.js
- `mysql2` untuk koneksi MySQL/MariaDB
- `dotenv` untuk konfigurasi environment
- `cors` bila frontend dan backend berjalan pada origin berbeda
- `express.json()` untuk membaca JSON

### Frontend
- Vue.js 3
- Vite
- JavaScript kecuali proyek secara eksplisit memilih TypeScript
- Fetch API atau Axios untuk memanggil backend
- Gunakan library grafik hanya jika kebutuhan visualisasi sudah jelas

### Database dan alat
- MySQL atau MariaDB melalui Laragon
- phpMyAdmin untuk inspeksi database
- Arduino IDE untuk program ESP32
- Git untuk version control

Jangan menambahkan dependensi besar tanpa alasan yang jelas.

## 3. Arsitektur

```text
ESP32
  │ HTTP POST (JSON) melalui Wi-Fi lokal
  ▼
Node.js + Express REST API
  │ validasi dan prepared query
  ▼
MySQL/MariaDB (Laragon)
  ▲
  │ HTTP GET
Vue.js 3 Dashboard
```

ESP32 **tidak** mengirim data langsung ke Vue.js atau database. ESP32 mengirim data ke endpoint backend. Frontend membaca data melalui API backend.

`localhost` pada ESP32 menunjuk ke ESP32 itu sendiri, bukan komputer server. Gunakan alamat IPv4 lokal komputer untuk URL API ESP32.

## 4. Struktur Direktori yang Disarankan

```text
esp32-monitoring/
├── README.md
├── backend/
│   ├── src/
│   │   ├── config/
│   │   │   └── db.js
│   │   ├── routes/
│   │   │   └── data.routes.js
│   │   ├── controllers/
│   │   │   └── data.controller.js
│   │   ├── services/
│   │   │   └── data.service.js
│   │   ├── middleware/
│   │   └── app.js
│   ├── server.js
│   ├── .env.example
│   ├── .gitignore
│   └── package.json
├── frontend/
│   ├── src/
│   │   ├── services/
│   │   │   └── api.js
│   │   ├── components/
│   │   ├── views/
│   │   ├── App.vue
│   │   └── main.js
│   ├── .env.example
│   ├── .gitignore
│   └── package.json
└── database/
    └── schema.sql
```

Agent boleh menyesuaikan struktur jika proyek yang sudah ada menggunakan pola berbeda. Jangan memindahkan atau menghapus file yang sudah ada tanpa memeriksa proyek terlebih dahulu.

## 5. Kontrak API Awal

Endpoint awal yang disarankan:

| Method | Endpoint | Fungsi |
|---|---|---|
| `GET` | `/api/health` | Memeriksa backend dan status dasar |
| `POST` | `/api/data` | Menerima data dari ESP32 |
| `GET` | `/api/data` | Mengambil daftar data dengan pagination |
| `GET` | `/api/data/:id` | Mengambil satu record berdasarkan ID |

Contoh payload:

```json
{
  "device_id": "esp32-01",
  "data": {
    "nilai": 123,
    "status": "aktif"
  }
}
```

Ketentuan:
- `device_id` harus berupa string non-kosong dengan panjang yang dibatasi.
- `data` harus berupa objek JSON.
- Tolak JSON tidak valid atau payload yang terlalu besar.
- Gunakan prepared statements, bukan menyusun SQL dengan menggabungkan input pengguna.
- Respons API konsisten menggunakan JSON dan HTTP status code yang sesuai.
- Respons sukses untuk data yang baru dibuat sebaiknya memakai HTTP `201`.
- Jangan mengembalikan detail error database atau stack trace kepada klien.

Contoh respons sukses:

```json
{
  "success": true,
  "message": "Data berhasil disimpan",
  "id": 1
}
```

Kontrak ini adalah titik awal dan boleh disesuaikan ketika jenis data ESP32 telah ditentukan.

## 6. Database

Gunakan database bernama `db_iot_esp32`. Skema awal yang fleksibel:

```sql
CREATE DATABASE IF NOT EXISTS db_iot_esp32
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE db_iot_esp32;

CREATE TABLE IF NOT EXISTS esp32_data (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    device_id VARCHAR(100) NOT NULL,
    data_json JSON NOT NULL,
    received_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_device_received (device_id, received_at)
);
```

MariaDB/MySQL memiliki perbedaan dukungan tipe JSON antarversi. Jika tipe `JSON` tidak tersedia atau berperilaku sebagai alias teks, gunakan `LONGTEXT` dan validasi JSON pada backend. Sesuaikan skema dengan versi database yang terpasang.

Konfigurasikan koneksi melalui environment variables. Jangan menanam kredensial database di source code.

Contoh `backend/.env.example`:

```dotenv
PORT=3000
HOST=0.0.0.0
DB_HOST=127.0.0.1
DB_PORT=3306
DB_NAME=db_iot_esp32
DB_USER=root
DB_PASSWORD=
CORS_ORIGIN=http://localhost:5173
```

Salin sebagai `.env` lokal dan isi kredensial yang benar. Jangan commit file `.env`.

## 7. Menjalankan Proyek Secara Lokal

### Prasyarat
- Node.js LTS dan npm
- Laragon dengan MySQL/MariaDB aktif
- Arduino IDE dan board ESP32 yang sesuai

### Database
1. Jalankan Laragon.
2. Aktifkan layanan MySQL/MariaDB.
3. Jalankan SQL di `database/schema.sql` melalui phpMyAdmin atau klien SQL.

### Backend
Setelah folder backend dibuat:

```bash
cd backend
npm install
```

Buat `.env` berdasarkan `.env.example`, lalu jalankan script yang tersedia di `package.json`, misalnya:

```bash
npm run dev
```

Pastikan server benar-benar mendengarkan pada port yang dikonfigurasi. Gunakan `127.0.0.1` untuk koneksi database lokal, dan `0.0.0.0` untuk server HTTP jika ESP32 perlu mengakses backend dari jaringan lokal.

### Frontend
```bash
cd frontend
npm install
npm run dev
```

Contoh `frontend/.env.example`:

```dotenv
VITE_API_BASE_URL=http://localhost:3000/api
```

Variabel dengan prefix `VITE_` terekspos ke browser. Jangan menyimpan rahasia di dalamnya.

### Jaringan ESP32
1. Hubungkan ESP32 dan komputer ke jaringan lokal yang sama.
2. Jalankan `ipconfig` di Windows dan temukan IPv4 komputer pada adaptor yang digunakan.
3. Atur URL API pada program ESP32, misalnya `http://192.168.1.10:3000/api/data` (alamat ini hanya contoh).
4. Pastikan firewall Windows mengizinkan koneksi masuk ke port backend.
5. Jangan menggunakan `localhost` atau `127.0.0.1` sebagai host komputer pada URL ESP32.

## 8. Instruksi Wajib untuk AI Coding Agent

Saat mengerjakan repositori ini:

1. **Periksa kondisi proyek terlebih dahulu.** Baca `package.json`, struktur folder, file konfigurasi, dan kode yang sudah ada sebelum mengubahnya.
2. Jangan menganggap proyek kosong. Pertahankan kode dan pola arsitektur yang sudah berjalan.
3. Buat perubahan kecil, terarah, dan konsisten dengan kebutuhan yang diminta.
4. Jangan mengganti stack utama: Node.js + Express.js, Vue.js 3 + Vite, dan MySQL/MariaDB Laragon, kecuali pengguna meminta perubahan.
5. Jangan mengasumsikan jenis data sensor atau perangkat. Gunakan payload contoh generik sampai pengguna memberikan spesifikasi.
6. Pisahkan routing, controller, layanan bisnis, dan konfigurasi database jika sesuai dengan ukuran proyek.
7. Gunakan `async/await`, penanganan error terpusat bila sesuai, dan prepared statements untuk query.
8. Validasi data dari perangkat. Jangan mempercayai payload yang masuk begitu saja.
9. Jangan mengekspos kredensial, `.env`, token, atau informasi sensitif. Pastikan `.env` masuk `.gitignore`.
10. Jangan menjalankan perintah destruktif, menghapus data, mereset database, atau mengubah konfigurasi global tanpa persetujuan pengguna.
11. Jangan menambahkan autentikasi, MQTT, WebSocket, Docker, atau library UI tambahan sebelum dibutuhkan atau diminta.
12. Setelah membuat perubahan, jelaskan file yang diubah, alasan perubahan, perintah untuk menjalankan, dan cara menguji hasilnya.
13. Jika memungkinkan, jalankan pemeriksaan atau tes yang relevan. Laporkan secara jujur tes yang berhasil dan yang belum dijalankan.
14. Jika ada informasi penting yang belum diketahui dan berpengaruh pada desain, tanyakan secara spesifik daripada membuat asumsi berisiko.

## 9. Keamanan dan Keandalan

- Gunakan prepared statements untuk semua query yang memakai input.
- Batasi ukuran request body JSON.
- Terapkan validasi payload dan tipe data.
- Batasi CORS ke origin frontend yang diperlukan; CORS bukan mekanisme autentikasi.
- Endpoint ESP32 pada jaringan lokal tetap dapat diakses perangkat lain di jaringan. Untuk penggunaan nyata, pertimbangkan API key atau autentikasi perangkat dan jangan menaruh rahasia permanen pada repositori publik.
- Jangan membuka port database ke jaringan umum. ESP32 hanya perlu mengakses port API backend.
- Untuk penggunaan di luar jaringan lokal, rancang autentikasi dan HTTPS dengan benar; jangan mengekspos endpoint HTTP lokal secara langsung ke internet.

## 10. Tahapan Pengembangan

1. Verifikasi Node.js, npm, dan Laragon.
2. Buat skema database dan uji koneksi backend.
3. Buat endpoint health check.
4. Implementasikan endpoint `POST /api/data`.
5. Uji API menggunakan PowerShell, curl, atau REST client sebelum menghubungkan perangkat.
6. Hubungkan ESP32 dan pastikan record masuk ke database.
7. Implementasikan endpoint daftar data dan pagination.
8. Buat dashboard Vue untuk status dan tabel data.
9. Tambahkan filter waktu, perangkat, grafik, atau ekspor bila dibutuhkan.
10. Dokumentasikan cara instalasi, konfigurasi, dan troubleshooting.

## 11. Kriteria Selesai Minimum

- Backend dapat berjalan lokal dan terhubung ke database Laragon.
- API menerima payload JSON yang valid dan menolak payload tidak valid.
- Data ESP32 dapat tersimpan di MySQL/MariaDB.
- Data yang tersimpan dapat dibaca kembali melalui API.
- Frontend Vue menampilkan data yang dikembalikan API.
- Konfigurasi lokal terdokumentasi tanpa membocorkan kredensial.
- Langkah pengujian dan troubleshooting dasar tersedia.

---

**Status proyek:** tahap perancangan awal. Jenis data ESP32 yang sebenarnya, detail sensor/perangkat, dan kebutuhan dashboard masih menunggu konfirmasi. Jangan menganggap data contoh sebagai spesifikasi final.
