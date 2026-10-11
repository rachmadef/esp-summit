// Deteksi URL API secara dinamis:
// Jika dibuka dari HP/perangkat lain di LAN (misal http://192.168.108.12:5173),
// otomatis hubungkan ke backend di http://192.168.108.12:3000/api
const getApiBaseUrl = () => {
  if (typeof window !== 'undefined' && window.location && window.location.hostname) {
    const port = '3000';
    const protocol = window.location.protocol;
    return `${protocol}//${window.location.hostname}:${port}/api`;
  }
  return import.meta.env.VITE_API_BASE_URL || 'http://localhost:3000/api';
};

const API_BASE_URL = getApiBaseUrl();

export const apiService = {
  getBaseUrl() {
    return API_BASE_URL;
  },

  async getHealth() {
    const res = await fetch(`${API_BASE_URL}/health`);
    if (!res.ok) {
      throw new Error(`Health check failed with status: ${res.status}`);
    }
    return res.json();
  },

  async getDataList({ page = 1, limit = 20, deviceId = '' } = {}) {
    const params = new URLSearchParams();
    if (page) params.set('page', page);
    if (limit) params.set('limit', limit);
    if (deviceId) params.set('device_id', deviceId);

    const url = `${API_BASE_URL}/data?${params.toString()}`;
    const res = await fetch(url);
    if (!res.ok) {
      throw new Error(`Gagal mengambil data (${res.status})`);
    }
    return res.json();
  },

  async getDataById(id) {
    const res = await fetch(`${API_BASE_URL}/data/${id}`);
    if (!res.ok) {
      throw new Error(`Gagal mengambil data #${id} (${res.status})`);
    }
    return res.json();
  },

  async sendData(payload) {
    const res = await fetch(`${API_BASE_URL}/data`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(payload)
    });

    const data = await res.json();
    if (!res.ok) {
      throw new Error(data.message || 'Gagal mengirim data');
    }
    return data;
  }
};
