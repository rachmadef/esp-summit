require('dotenv').config();
const app = require('./app');
const pool = require('./config/db');

const PORT = parseInt(process.env.PORT, 10) || 3000;
const HOST = process.env.HOST || '0.0.0.0';

async function startServer() {
  try {
    // Verifikasi koneksi awal ke database
    const connection = await pool.getConnection();
    console.log(`[Database] Terhubung ke MySQL/MariaDB (${process.env.DB_NAME || 'db_iot_esp32'}) di ${process.env.DB_HOST || '127.0.0.1'}:${process.env.DB_PORT || 3306}`);
    connection.release();

    app.listen(PORT, HOST, () => {
      console.log(`[Server] Berjalan di http://${HOST}:${PORT}`);
      console.log(`[Server] Endpoint API: http://${HOST}:${PORT}/api/data`);
      console.log(`[Server] Health Check: http://${HOST}:${PORT}/api/health`);
    });
  } catch (error) {
    console.error('[Database] Gagal terhubung ke database:', error.message);
    console.log('[Server] Memulai server dalam mode terbatas...');
    app.listen(PORT, HOST, () => {
      console.log(`[Server] Berjalan di http://${HOST}:${PORT} (Database belum terhubung)`);
    });
  }
}

startServer();
