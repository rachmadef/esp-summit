const pool = require('../config/db');

/**
 * Normalisasi data_json jika mysql2 mengembalikannya sebagai string
 */
function parseDataJson(record) {
  if (!record) return null;
  if (typeof record.data_json === 'string') {
    try {
      record.data_json = JSON.parse(record.data_json);
    } catch {
      // biarkan tetap string jika gagal parse
    }
  }
  return record;
}

const dataService = {
  async checkDatabase() {
    const [rows] = await pool.query('SELECT 1 AS status');
    return rows[0]?.status === 1;
  },

  async saveData(deviceId, dataObject) {
    const sql = `
      INSERT INTO esp32_data (device_id, data_json)
      VALUES (?, ?)
    `;
    const jsonString = JSON.stringify(dataObject);
    const [result] = await pool.execute(sql, [deviceId, jsonString]);
    return result.insertId;
  },

  async getDataList({ page = 1, limit = 20, deviceId = null }) {
    const pageNum = Math.max(1, parseInt(page, 10) || 1);
    const limitNum = Math.max(1, Math.min(100, parseInt(limit, 10) || 20));
    const offset = (pageNum - 1) * limitNum;

    let whereClause = '';
    const params = [];

    if (deviceId) {
      whereClause = 'WHERE device_id = ?';
      params.push(deviceId);
    }

    // Query Total
    const countSql = `SELECT COUNT(*) AS total FROM esp32_data ${whereClause}`;
    const [countRows] = await pool.execute(countSql, params);
    const total = countRows[0]?.total || 0;

    // Query Data
    const dataSql = `
      SELECT id, device_id, data_json, received_at
      FROM esp32_data
      ${whereClause}
      ORDER BY received_at DESC, id DESC
      LIMIT ? OFFSET ?
    `;

    // Note: limit dan offset di mysql2 execute harus integer number
    const [rows] = await pool.query(dataSql, [...params, limitNum, offset]);

    const formattedRows = rows.map(parseDataJson);

    return {
      total,
      page: pageNum,
      limit: limitNum,
      totalPages: Math.ceil(total / limitNum),
      data: formattedRows
    };
  },

  async getDataById(id) {
    const sql = `
      SELECT id, device_id, data_json, received_at
      FROM esp32_data
      WHERE id = ?
      LIMIT 1
    `;
    const [rows] = await pool.execute(sql, [id]);
    if (rows.length === 0) {
      return null;
    }
    return parseDataJson(rows[0]);
  }
};

module.exports = dataService;
