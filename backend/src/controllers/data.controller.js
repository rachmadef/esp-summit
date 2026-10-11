const dataService = require('../services/data.service');

const dataController = {
  async healthCheck(req, res) {
    try {
      const dbConnected = await dataService.checkDatabase();
      return res.status(200).json({
        success: true,
        status: 'healthy',
        database: dbConnected ? 'connected' : 'disconnected',
        timestamp: new Date().toISOString()
      });
    } catch (error) {
      console.error('Health check error:', error.message);
      return res.status(503).json({
        success: false,
        status: 'unhealthy',
        database: 'disconnected',
        message: 'Koneksi database bermasalah'
      });
    }
  },

  async createData(req, res) {
    try {
      const { device_id, data } = req.body || {};

      // Validasi device_id
      if (!device_id || typeof device_id !== 'string' || device_id.trim().length === 0) {
        return res.status(400).json({
          success: false,
          message: 'device_id harus berupa string non-kosong'
        });
      }

      const trimmedDeviceId = device_id.trim();
      if (trimmedDeviceId.length > 100) {
        return res.status(400).json({
          success: false,
          message: 'device_id maksimal 100 karakter'
        });
      }

      // Validasi data (harus berupa objek JSON, bukan array, bukan null)
      if (!data || typeof data !== 'object' || Array.isArray(data)) {
        return res.status(400).json({
          success: false,
          message: 'data harus berupa objek JSON'
        });
      }

      const insertId = await dataService.saveData(trimmedDeviceId, data);

      return res.status(201).json({
        success: true,
        message: 'Data berhasil disimpan',
        id: insertId
      });
    } catch (error) {
      console.error('Error menyimpan data ESP32:', error.message);
      return res.status(500).json({
        success: false,
        message: 'Gagal menyimpan data ke database'
      });
    }
  },

  async getAllData(req, res) {
    try {
      const { page, limit, device_id } = req.query;
      const result = await dataService.getDataList({
        page,
        limit,
        deviceId: device_id ? device_id.trim() : null
      });

      return res.status(200).json({
        success: true,
        ...result
      });
    } catch (error) {
      console.error('Error mengambil data ESP32:', error.message);
      return res.status(500).json({
        success: false,
        message: 'Gagal mengambil data dari database'
      });
    }
  },

  async getDataById(req, res) {
    try {
      const id = parseInt(req.params.id, 10);
      if (isNaN(id) || id <= 0) {
        return res.status(400).json({
          success: false,
          message: 'ID tidak valid'
        });
      }

      const record = await dataService.getDataById(id);
      if (!record) {
        return res.status(404).json({
          success: false,
          message: 'Data tidak ditemukan'
        });
      }

      return res.status(200).json({
        success: true,
        data: record
      });
    } catch (error) {
      console.error('Error mengambil data berdasarkan ID:', error.message);
      return res.status(500).json({
        success: false,
        message: 'Gagal mengambil data'
      });
    }
  }
};

module.exports = dataController;
