const express = require('express');
const cors = require('cors');
const dataRoutes = require('./routes/data.routes');

const app = express();

// Konfigurasi CORS
const allowedOrigin = process.env.CORS_ORIGIN || 'http://localhost:5173';
app.use(cors({
  origin: (origin, callback) => {
    // Izinkan request tanpa origin (seperti dari ESP32, curl, atau Postman)
    // dan izinkan origin frontend yang cocok
    if (!origin || origin === allowedOrigin || origin.startsWith('http://localhost:')) {
      return callback(null, true);
    }
    return callback(null, true); // Untuk fleksibilitas akses jaringan LAN
  },
  credentials: true
}));

// Body parser dengan batasan ukuran 100kb
app.use(express.json({ limit: '100kb' }));

// Routing API
app.use('/api', dataRoutes);

// Root test
app.get('/', (req, res) => {
  res.json({
    message: 'ESP32 Monitoring System API is running',
    docs: '/api/health'
  });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({
    success: false,
    message: `Rute ${req.method} ${req.originalUrl} tidak ditemukan`
  });
});

// Centralized error handler
app.use((err, req, res, next) => {
  // Tangani error parsing JSON
  if (err instanceof SyntaxError && err.status === 400 && 'body' in err) {
    return res.status(400).json({
      success: false,
      message: 'Format payload JSON tidak valid'
    });
  }

  // Tangani payload too large
  if (err.type === 'entity.too.large') {
    return res.status(413).json({
      success: false,
      message: 'Ukuran payload melebihi batas yang diizinkan'
    });
  }

  console.error('Unhandled Server Error:', err.message);
  return res.status(500).json({
    success: false,
    message: 'Terjadi kesalahan pada server'
  });
});

module.exports = app;
