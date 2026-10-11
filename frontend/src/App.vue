<script setup>
import { ref, onMounted, onUnmounted, computed, watch } from 'vue';
import { apiService } from './services/api';

// State Sistem & Koneksi
const isHealthLoading = ref(false);
const healthStatus = ref({
  success: false,
  status: 'checking',
  database: 'checking'
});

// State Data & Tabel
const isLoadingData = ref(false);
const dataList = ref([]);
const totalRecords = ref(0);
const currentPage = ref(1);
const limitPerPage = ref(10);
const totalPages = ref(1);
const filterDeviceId = ref('');
const errorMessage = ref('');

// Auto Refresh Timer
const refreshIntervalSec = ref(5);
let timerId = null;

// Modal Detail JSON
const selectedRecord = ref(null);
const showDetailModal = ref(false);

// Modal Simulator ESP32
const showSimulatorModal = ref(false);
const simulatorDeviceId = ref('esp32-ruang-utama');
const simulatorPayloadText = ref(JSON.stringify({
  suhu: 27.8,
  kelembaban: 65,
  tegangan: 3.3,
  status: 'normal'
}, null, 2));
const simulatorStatus = ref({ loading: false, success: null, message: '' });

// Salin status copy
const isCopied = ref(false);

// Perangkat Unik dari data yang dimuat
const uniqueDevicesCount = computed(() => {
  const set = new Set(dataList.value.map(d => d.device_id));
  return set.size;
});

// Record terbaru
const latestRecordTime = computed(() => {
  if (dataList.value.length === 0) return '-';
  const first = dataList.value[0];
  return formatDate(first.received_at);
});

// Format Waktu
function formatDate(dateString) {
  if (!dateString) return '-';
  try {
    const d = new Date(dateString);
    return d.toLocaleString('id-ID', {
      day: '2-digit',
      month: 'short',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    });
  } catch {
    return dateString;
  }
}

// Cek Health Backend & Database
async function checkHealth() {
  isHealthLoading.value = true;
  try {
    const res = await apiService.getHealth();
    healthStatus.value = res;
  } catch (err) {
    healthStatus.value = {
      success: false,
      status: 'offline',
      database: 'disconnected',
      message: err.message
    };
  } finally {
    isHealthLoading.value = false;
  }
}

// Ambil Daftar Data
async function loadData() {
  isLoadingData.value = true;
  errorMessage.value = '';
  try {
    const res = await apiService.getDataList({
      page: currentPage.value,
      limit: limitPerPage.value,
      deviceId: filterDeviceId.value
    });

    dataList.value = res.data || [];
    totalRecords.value = res.total || 0;
    totalPages.value = res.totalPages || 1;
  } catch (err) {
    errorMessage.value = err.message || 'Gagal memuat data dari backend';
  } finally {
    isLoadingData.value = false;
  }
}

// Muat Ulang Semua (Health + Data)
async function refreshAll() {
  await Promise.all([checkHealth(), loadData()]);
}

// Setup Auto Refresh
function setupAutoRefresh() {
  if (timerId) clearInterval(timerId);
  const sec = parseInt(refreshIntervalSec.value, 10);
  if (sec > 0) {
    timerId = setInterval(() => {
      refreshAll();
    }, sec * 1000);
  }
}

watch(refreshIntervalSec, () => {
  setupAutoRefresh();
});

// Filter & Pagination Actions
function handleFilterSubmit() {
  currentPage.value = 1;
  loadData();
}

function resetFilter() {
  filterDeviceId.value = '';
  currentPage.value = 1;
  loadData();
}

function goToPage(page) {
  if (page >= 1 && page <= totalPages.value) {
    currentPage.value = page;
    loadData();
  }
}

// Modal Detail JSON
function openDetail(item) {
  selectedRecord.value = item;
  showDetailModal.value = true;
}

function closeDetail() {
  showDetailModal.value = false;
  selectedRecord.value = null;
}

async function copyJsonToClipboard(data) {
  try {
    await navigator.clipboard.writeText(JSON.stringify(data, null, 2));
    isCopied.value = true;
    setTimeout(() => {
      isCopied.value = false;
    }, 2000);
  } catch {
    // fallback
  }
}

// Kirim Simulasi Payload ESP32
async function submitSimulator() {
  simulatorStatus.value = { loading: true, success: null, message: '' };

  let parsedData;
  try {
    parsedData = JSON.parse(simulatorPayloadText.value);
    if (typeof parsedData !== 'object' || Array.isArray(parsedData) || parsedData === null) {
      throw new Error('Payload harus berupa objek JSON (contoh: { "kunci": "nilai" })');
    }
  } catch (e) {
    simulatorStatus.value = {
      loading: false,
      success: false,
      message: 'Format JSON tidak valid: ' + e.message
    };
    return;
  }

  try {
    const res = await apiService.sendData({
      device_id: simulatorDeviceId.value,
      data: parsedData
    });

    simulatorStatus.value = {
      loading: false,
      success: true,
      message: `Sukses! Data tersimpan dengan ID: #${res.id}`
    };

    // Refresh tabel otomatis
    await loadData();
    setTimeout(() => {
      if (simulatorStatus.value.success) {
        showSimulatorModal.value = false;
        simulatorStatus.value = { loading: false, success: null, message: '' };
      }
    }, 1200);
  } catch (err) {
    simulatorStatus.value = {
      loading: false,
      success: false,
      message: err.message || 'Gagal mengirim data'
    };
  }
}

// Contoh Template Simulasi
function setSimulatorTemplate(type) {
  if (type === 'suhu') {
    simulatorPayloadText.value = JSON.stringify({
      suhu_celsius: 28.4,
      kelembaban_persen: 62.0,
      tekanan_hpa: 1013.2
    }, null, 2);
  } else if (type === 'tegangan') {
    simulatorPayloadText.value = JSON.stringify({
      tegangan_volt: 3.32,
      arus_ma: 145,
      baterai_persen: 88,
      status: 'aktif'
    }, null, 2);
  } else if (type === 'gerakan') {
    simulatorPayloadText.value = JSON.stringify({
      sensor_pir: true,
      intensitas_lux: 450,
      pintu_terbuka: false
    }, null, 2);
  }
}

onMounted(() => {
  refreshAll();
  setupAutoRefresh();
});

onUnmounted(() => {
  if (timerId) clearInterval(timerId);
});
</script>

<template>
  <div class="dashboard-wrapper">
    <!-- Navbar / Header -->
    <header class="navbar">
      <div class="navbar-left">
        <div class="brand-logo">
          <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"></path>
          </svg>
        </div>
        <div>
          <h1 class="navbar-title">ESP32 Monitoring Dashboard</h1>
          <p class="navbar-subtitle">Node.js Express &bull; MySQL Laragon &bull; Vue.js 3</p>
        </div>
      </div>

      <div class="navbar-right">
        <!-- Status Badges -->
        <div class="status-indicators">
          <span class="status-pill" :class="healthStatus.status === 'healthy' ? 'pill-online' : 'pill-offline'">
            <span class="dot"></span>
            API: {{ healthStatus.status === 'healthy' ? 'Online' : 'Offline' }}
          </span>

          <span class="status-pill" :class="healthStatus.database === 'connected' ? 'pill-online' : 'pill-offline'">
            <span class="dot"></span>
            DB: {{ healthStatus.database === 'connected' ? 'Terhubung' : 'Terputus' }}
          </span>
        </div>

        <!-- Auto Refresh Selector -->
        <div class="refresh-control">
          <label for="auto-refresh-select" class="control-label">Auto-refresh:</label>
          <select id="auto-refresh-select" v-model="refreshIntervalSec" class="select-input">
            <option :value="0">Nonaktif</option>
            <option :value="3">3 Detik</option>
            <option :value="5">5 Detik</option>
            <option :value="10">10 Detik</option>
            <option :value="30">30 Detik</option>
          </select>
        </div>

        <!-- Action Buttons -->
        <button id="btn-refresh" class="btn btn-secondary" @click="refreshAll" :disabled="isLoadingData">
          <svg :class="{ 'spin-icon': isLoadingData }" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polyline points="23 4 23 10 17 10"></polyline>
            <polyline points="1 20 1 14 7 14"></polyline>
            <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path>
          </svg>
          Segarkan
        </button>

        <button id="btn-open-simulator" class="btn btn-primary" @click="showSimulatorModal = true">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polygon points="5 3 19 12 5 21 5 3"></polygon>
          </svg>
          Simulator ESP32
        </button>
      </div>
    </header>

    <!-- Main Container -->
    <main class="main-content">
      <!-- Error Banner -->
      <div v-if="errorMessage" class="alert-box alert-error">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="10"></circle>
          <line x1="12" y1="8" x2="12" y2="12"></line>
          <line x1="12" y1="16" x2="12.01" y2="16"></line>
        </svg>
        <span>{{ errorMessage }}</span>
        <button class="alert-close" @click="errorMessage = ''">&times;</button>
      </div>

      <!-- Stat Cards -->
      <section class="stats-grid">
        <div class="stat-card">
          <div class="stat-header">
            <span class="stat-label">Total Data Tercatat</span>
            <span class="stat-icon icon-cyan">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <ellipse cx="12" cy="5" rx="9" ry="3"></ellipse>
                <path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3"></path>
                <path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5"></path>
              </svg>
            </span>
          </div>
          <div class="stat-value">{{ totalRecords }}</div>
          <div class="stat-footer">Tersimpan di tabel `esp32_data`</div>
        </div>

        <div class="stat-card">
          <div class="stat-header">
            <span class="stat-label">Perangkat Aktif (Halaman Ini)</span>
            <span class="stat-icon icon-emerald">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect>
                <rect x="9" y="9" width="6" height="6"></rect>
                <line x1="9" y1="1" x2="9" y2="4"></line>
                <line x1="15" y1="1" x2="15" y2="4"></line>
                <line x1="9" y1="20" x2="9" y2="23"></line>
                <line x1="15" y1="20" x2="15" y2="23"></line>
                <line x1="20" y1="9" x2="23" y2="9"></line>
                <line x1="20" y1="14" x2="23" y2="14"></line>
                <line x1="1" y1="9" x2="4" y2="9"></line>
                <line x1="1" y1="14" x2="4" y2="14"></line>
              </svg>
            </span>
          </div>
          <div class="stat-value">{{ uniqueDevicesCount }}</div>
          <div class="stat-footer">Device ID berbeda</div>
        </div>

        <div class="stat-card">
          <div class="stat-header">
            <span class="stat-label">Waktu Data Terakhir</span>
            <span class="stat-icon icon-indigo">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"></circle>
                <polyline points="12 6 12 12 16 14"></polyline>
              </svg>
            </span>
          </div>
          <div class="stat-value stat-value-sm">{{ latestRecordTime }}</div>
          <div class="stat-footer">Terakhir diterima server</div>
        </div>

        <div class="stat-card">
          <div class="stat-header">
            <span class="stat-label">Info Jaringan ESP32</span>
            <span class="stat-icon icon-amber">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
                <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
                <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
                <line x1="12" y1="20" x2="12.01" y2="20"></line>
              </svg>
            </span>
          </div>
          <div class="stat-value stat-value-sm code-font">PORT: 3000 / API: /api/data</div>
          <div class="stat-footer">Gunakan IP Komputer (bukan localhost)</div>
        </div>
      </section>

      <!-- Data Section with Filters -->
      <section class="table-section">
        <div class="section-toolbar">
          <div class="toolbar-title-group">
            <h2 class="section-title">Riwayat Telemetri Data</h2>
            <span class="section-badge">Halaman {{ currentPage }} dari {{ totalPages }}</span>
          </div>

          <div class="toolbar-controls">
            <!-- Filter Form -->
            <form class="search-form" @submit.prevent="handleFilterSubmit">
              <div class="search-input-wrap">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <circle cx="11" cy="11" r="8"></circle>
                  <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
                </svg>
                <input
                  v-model="filterDeviceId"
                  type="text"
                  placeholder="Filter Device ID..."
                  class="search-input"
                />
              </div>
              <button type="submit" class="btn btn-secondary btn-sm">Cari</button>
              <button v-if="filterDeviceId" type="button" class="btn btn-ghost btn-sm" @click="resetFilter">Reset</button>
            </form>

            <!-- Limit per Page -->
            <div class="limit-select-wrap">
              <label for="limit-select" class="control-label">Limit:</label>
              <select id="limit-select" v-model="limitPerPage" @change="loadData" class="select-input select-sm">
                <option :value="10">10</option>
                <option :value="20">20</option>
                <option :value="50">50</option>
              </select>
            </div>
          </div>
        </div>

        <!-- Table Container -->
        <div class="table-card">
          <div v-if="isLoadingData" class="table-loader">
            <div class="spinner"></div>
            <span>Memuat data telemetri...</span>
          </div>

          <div v-else-if="dataList.length === 0" class="empty-state">
            <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
              <circle cx="12" cy="12" r="10"></circle>
              <line x1="8" y1="12" x2="16" y2="12"></line>
            </svg>
            <h3>Belum Ada Data</h3>
            <p>Belum ada data ESP32 yang tersimpan di database. Anda dapat mengirimkan data dari perangkat fisik atau mencoba dengan simulator.</p>
            <button class="btn btn-primary btn-sm" @click="showSimulatorModal = true">Kirim Data Contoh</button>
          </div>

          <div v-else class="table-responsive">
            <table class="data-table">
              <thead>
                <tr>
                  <th style="width: 80px;">ID</th>
                  <th style="width: 180px;">Device ID</th>
                  <th>Payload JSON Data</th>
                  <th style="width: 220px;">Waktu Diterima</th>
                  <th style="width: 110px; text-align: center;">Aksi</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="item in dataList" :key="item.id">
                  <td class="code-font text-muted">#{{ item.id }}</td>
                  <td>
                    <span class="device-badge">
                      <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <circle cx="12" cy="12" r="2"></circle>
                        <path d="M16.24 7.76a6 6 0 0 1 0 8.49m-8.48-.01a6 6 0 0 1 0-8.49m11.31-2.82a10 10 0 0 1 0 14.14m-14.14 0a10 10 0 0 1 0-14.14"></path>
                      </svg>
                      {{ item.device_id }}
                    </span>
                  </td>
                  <td>
                    <div class="json-preview-container">
                      <span v-for="(val, key) in item.data_json" :key="key" class="json-tag">
                        <strong>{{ key }}:</strong> {{ typeof val === 'object' ? JSON.stringify(val) : val }}
                      </span>
                    </div>
                  </td>
                  <td class="text-muted font-sm">
                    {{ formatDate(item.received_at) }}
                  </td>
                  <td style="text-align: center;">
                    <button class="btn btn-ghost btn-sm" @click="openDetail(item)" title="Lihat detail JSON">
                      Detail
                    </button>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>

          <!-- Pagination Footer -->
          <div v-if="totalPages > 1" class="pagination-bar">
            <div class="pagination-info">
              Menampilkan {{ dataList.length }} dari {{ totalRecords }} total rekaman
            </div>
            <div class="pagination-controls">
              <button
                class="btn btn-ghost btn-sm"
                :disabled="currentPage <= 1"
                @click="goToPage(currentPage - 1)"
              >
                &larr; Sebelumnya
              </button>

              <div class="page-numbers">
                <button
                  v-for="p in totalPages"
                  :key="p"
                  class="page-number-btn"
                  :class="{ active: p === currentPage }"
                  @click="goToPage(p)"
                >
                  {{ p }}
                </button>
              </div>

              <button
                class="btn btn-ghost btn-sm"
                :disabled="currentPage >= totalPages"
                @click="goToPage(currentPage + 1)"
              >
                Selanjutnya &rarr;
              </button>
            </div>
          </div>
        </div>
      </section>
    </main>

    <!-- Modal Detail Record -->
    <div v-if="showDetailModal && selectedRecord" class="modal-backdrop" @click.self="closeDetail">
      <div class="modal-card">
        <div class="modal-header">
          <div class="modal-title-group">
            <h3>Detail Record #{{ selectedRecord.id }}</h3>
            <span class="device-badge">{{ selectedRecord.device_id }}</span>
          </div>
          <button class="modal-close" @click="closeDetail">&times;</button>
        </div>

        <div class="modal-body">
          <div class="meta-row">
            <div>
              <span class="meta-label">Waktu Diterima:</span>
              <span class="meta-value">{{ formatDate(selectedRecord.received_at) }}</span>
            </div>
            <button class="btn btn-secondary btn-sm" @click="copyJsonToClipboard(selectedRecord.data_json)">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect>
                <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path>
              </svg>
              {{ isCopied ? 'Tersalin!' : 'Salin JSON' }}
            </button>
          </div>

          <div class="json-code-box">
            <pre><code>{{ JSON.stringify(selectedRecord.data_json, null, 2) }}</code></pre>
          </div>
        </div>

        <div class="modal-footer">
          <button class="btn btn-secondary" @click="closeDetail">Tutup</button>
        </div>
      </div>
    </div>

    <!-- Modal Simulator ESP32 -->
    <div v-if="showSimulatorModal" class="modal-backdrop" @click.self="showSimulatorModal = false">
      <div class="modal-card">
        <div class="modal-header">
          <div>
            <h3>Simulator Pengiriman Data ESP32</h3>
            <p class="modal-subtitle">Menguji endpoint POST /api/data tanpa perangkat fisik</p>
          </div>
          <button class="modal-close" @click="showSimulatorModal = false">&times;</button>
        </div>

        <div class="modal-body">
          <!-- Template selector -->
          <div class="form-group">
            <label class="form-label">Gunakan Template Contoh:</label>
            <div class="template-buttons">
              <button type="button" class="btn btn-ghost btn-xs" @click="setSimulatorTemplate('suhu')">Suhu & Kelembaban</button>
              <button type="button" class="btn btn-ghost btn-xs" @click="setSimulatorTemplate('tegangan')">Baterai & Tegangan</button>
              <button type="button" class="btn btn-ghost btn-xs" @click="setSimulatorTemplate('gerakan')">Sensor Gerak/PIR</button>
            </div>
          </div>

          <!-- Device ID Input -->
          <div class="form-group">
            <label for="sim-device-id" class="form-label">Device ID:</label>
            <input
              id="sim-device-id"
              v-model="simulatorDeviceId"
              type="text"
              class="form-input"
              placeholder="Contoh: esp32-01"
            />
          </div>

          <!-- Payload Textarea -->
          <div class="form-group">
            <label for="sim-payload" class="form-label">Payload JSON (Objek):</label>
            <textarea
              id="sim-payload"
              v-model="simulatorPayloadText"
              rows="7"
              class="form-textarea code-font"
              placeholder="{\n  &quot;kunci&quot;: &quot;nilai&quot;\n}"
            ></textarea>
          </div>

          <!-- Status / Feedback -->
          <div v-if="simulatorStatus.message" class="alert-box" :class="simulatorStatus.success ? 'alert-success' : 'alert-error'">
            {{ simulatorStatus.message }}
          </div>
        </div>

        <div class="modal-footer">
          <button class="btn btn-secondary" @click="showSimulatorModal = false">Batal</button>
          <button
            class="btn btn-primary"
            :disabled="simulatorStatus.loading || !simulatorDeviceId"
            @click="submitSimulator"
          >
            <svg v-if="simulatorStatus.loading" class="spin-icon" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <line x1="12" y1="2" x2="12" y2="6"></line>
              <line x1="12" y1="18" x2="12" y2="22"></line>
            </svg>
            {{ simulatorStatus.loading ? 'Mengirim...' : 'Kirim Payload' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* Reset & Base Token */
:root {
  --bg-dark: #0b0f19;
  --bg-card: #111827;
  --bg-card-subtle: #1f2937;
  --border-color: rgba(255, 255, 255, 0.08);
  --text-main: #f9fafb;
  --text-muted: #9ca3af;
  --accent-primary: #3b82f6;
  --accent-primary-hover: #2563eb;
  --accent-emerald: #10b981;
  --accent-rose: #f43f5e;
  --accent-amber: #f59e0b;
  --accent-cyan: #06b6d4;
  --accent-indigo: #6366f1;
}

.dashboard-wrapper {
  min-height: 100vh;
  background: radial-gradient(circle at 10% 20%, #0f172a 0%, #030712 100%);
  color: #f3f4f6;
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  padding: 0;
  margin: 0;
  box-sizing: border-box;
}

/* Navbar */
.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1rem 2rem;
  background: rgba(17, 24, 39, 0.75);
  backdrop-filter: blur(12px);
  border-bottom: 1px solid rgba(255, 255, 255, 0.08);
  position: sticky;
  top: 0;
  z-index: 50;
  flex-wrap: wrap;
  gap: 1rem;
}

.navbar-left {
  display: flex;
  align-items: center;
  gap: 0.85rem;
}

.brand-logo {
  width: 40px;
  height: 40px;
  border-radius: 10px;
  background: linear-gradient(135deg, #2563eb, #38bdf8);
  display: flex;
  align-items: center;
  justify-content: center;
  color: #fff;
  box-shadow: 0 4px 12px rgba(37, 99, 235, 0.35);
}

.navbar-title {
  font-size: 1.15rem;
  font-weight: 700;
  letter-spacing: -0.02em;
  margin: 0;
  color: #ffffff;
}

.navbar-subtitle {
  font-size: 0.75rem;
  color: #94a3b8;
  margin: 0.15rem 0 0 0;
}

.navbar-right {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

/* Status Indicators */
.status-indicators {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.status-pill {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.35rem 0.65rem;
  border-radius: 9999px;
  font-size: 0.75rem;
  font-weight: 600;
  border: 1px solid transparent;
}

.pill-online {
  background: rgba(16, 185, 129, 0.12);
  color: #34d399;
  border-color: rgba(16, 185, 129, 0.25);
}

.pill-offline {
  background: rgba(244, 63, 94, 0.12);
  color: #fb7185;
  border-color: rgba(244, 63, 94, 0.25);
}

.dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: currentColor;
}

/* Refresh Control */
.refresh-control {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.control-label {
  font-size: 0.78rem;
  color: #94a3b8;
}

.select-input {
  background: #1e293b;
  color: #f1f5f9;
  border: 1px solid rgba(255, 255, 255, 0.12);
  padding: 0.4rem 0.6rem;
  border-radius: 6px;
  font-size: 0.8rem;
  outline: none;
  cursor: pointer;
}

.select-sm {
  padding: 0.25rem 0.5rem;
  font-size: 0.75rem;
}

/* Buttons */
.btn {
  display: inline-flex;
  align-items: center;
  gap: 0.45rem;
  padding: 0.5rem 0.85rem;
  border-radius: 6px;
  font-size: 0.82rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
  border: 1px solid transparent;
}

.btn-primary {
  background: linear-gradient(135deg, #2563eb, #1d4ed8);
  color: #ffffff;
  box-shadow: 0 2px 8px rgba(37, 99, 235, 0.3);
}

.btn-primary:hover:not(:disabled) {
  background: linear-gradient(135deg, #1d4ed8, #1e40af);
}

.btn-secondary {
  background: #1e293b;
  color: #e2e8f0;
  border-color: rgba(255, 255, 255, 0.12);
}

.btn-secondary:hover:not(:disabled) {
  background: #334155;
}

.btn-ghost {
  background: transparent;
  color: #94a3b8;
}

.btn-ghost:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.06);
  color: #ffffff;
}

.btn-sm {
  padding: 0.3rem 0.6rem;
  font-size: 0.78rem;
}

.btn-xs {
  padding: 0.25rem 0.5rem;
  font-size: 0.72rem;
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.spin-icon {
  animation: spin 1s linear infinite;
}

@keyframes spin {
  100% { transform: rotate(360deg); }
}

/* Main Content */
.main-content {
  max-width: 1320px;
  margin: 0 auto;
  padding: 1.75rem 2rem 3rem 2rem;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

/* Alert Box */
.alert-box {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.85rem 1.25rem;
  border-radius: 8px;
  font-size: 0.85rem;
}

.alert-error {
  background: rgba(244, 63, 94, 0.1);
  border: 1px solid rgba(244, 63, 94, 0.3);
  color: #fda4af;
}

.alert-success {
  background: rgba(16, 185, 129, 0.1);
  border: 1px solid rgba(16, 185, 129, 0.3);
  color: #6ee7b7;
}

.alert-close {
  margin-left: auto;
  background: none;
  border: none;
  color: inherit;
  font-size: 1.25rem;
  cursor: pointer;
}

/* Stats Grid */
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 1.25rem;
}

.stat-card {
  background: rgba(30, 41, 59, 0.5);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 12px;
  padding: 1.25rem;
  backdrop-filter: blur(8px);
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  transition: transform 0.2s ease, border-color 0.2s ease;
}

.stat-card:hover {
  transform: translateY(-2px);
  border-color: rgba(255, 255, 255, 0.16);
}

.stat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-size: 0.8rem;
  color: #94a3b8;
  font-weight: 500;
}

.stat-icon {
  width: 32px;
  height: 32px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.icon-cyan { background: rgba(6, 182, 212, 0.15); color: #22d3ee; }
.icon-emerald { background: rgba(16, 185, 129, 0.15); color: #34d399; }
.icon-indigo { background: rgba(99, 102, 241, 0.15); color: #818cf8; }
.icon-amber { background: rgba(245, 158, 11, 0.15); color: #fbbf24; }

.stat-value {
  font-size: 1.85rem;
  font-weight: 700;
  color: #ffffff;
}

.stat-value-sm {
  font-size: 0.95rem;
  font-weight: 600;
  word-break: break-all;
}

.stat-footer {
  font-size: 0.72rem;
  color: #64748b;
}

/* Table Section */
.table-section {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.section-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 1rem;
}

.toolbar-title-group {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.section-title {
  font-size: 1.15rem;
  font-weight: 600;
  color: #ffffff;
  margin: 0;
}

.section-badge {
  font-size: 0.75rem;
  background: rgba(255, 255, 255, 0.08);
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
  color: #94a3b8;
}

.toolbar-controls {
  display: flex;
  align-items: center;
  gap: 0.85rem;
  flex-wrap: wrap;
}

.search-form {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.search-input-wrap {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  background: #1e293b;
  border: 1px solid rgba(255, 255, 255, 0.12);
  padding: 0.35rem 0.7rem;
  border-radius: 6px;
  color: #94a3b8;
}

.search-input {
  background: transparent;
  border: none;
  outline: none;
  color: #f1f5f9;
  font-size: 0.82rem;
  width: 170px;
}

.limit-select-wrap {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

/* Table Card */
.table-card {
  background: rgba(30, 41, 59, 0.45);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 12px;
  backdrop-filter: blur(8px);
  overflow: hidden;
}

.table-responsive {
  overflow-x: auto;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
  text-align: left;
  font-size: 0.85rem;
}

.data-table th {
  background: rgba(15, 23, 42, 0.65);
  color: #94a3b8;
  font-weight: 600;
  padding: 0.85rem 1.25rem;
  border-bottom: 1px solid rgba(255, 255, 255, 0.08);
  font-size: 0.75rem;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.data-table td {
  padding: 0.85rem 1.25rem;
  border-bottom: 1px solid rgba(255, 255, 255, 0.05);
  color: #e2e8f0;
}

.data-table tr:hover td {
  background: rgba(255, 255, 255, 0.02);
}

.device-badge {
  display: inline-flex;
  align-items: center;
  gap: 0.35rem;
  background: rgba(59, 130, 246, 0.12);
  color: #60a5fa;
  padding: 0.25rem 0.55rem;
  border-radius: 6px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 0.78rem;
  font-weight: 500;
}

.json-preview-container {
  display: flex;
  flex-wrap: wrap;
  gap: 0.35rem;
}

.json-tag {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.08);
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-family: 'JetBrains Mono', monospace;
  color: #cbd5e1;
}

.json-tag strong {
  color: #38bdf8;
}

.code-font {
  font-family: 'JetBrains Mono', monospace;
}

.text-muted {
  color: #94a3b8;
}

.font-sm {
  font-size: 0.8rem;
}

.table-loader {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 0.75rem;
  padding: 4rem;
  color: #94a3b8;
}

.spinner {
  width: 28px;
  height: 28px;
  border: 3px solid rgba(255, 255, 255, 0.1);
  border-top-color: #3b82f6;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 4rem 2rem;
  text-align: center;
  color: #64748b;
  gap: 0.5rem;
}

.empty-state h3 {
  color: #e2e8f0;
  margin: 0.5rem 0 0 0;
}

.empty-state p {
  max-width: 420px;
  font-size: 0.85rem;
  margin: 0 0 1rem 0;
}

/* Pagination Bar */
.pagination-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.85rem 1.25rem;
  background: rgba(15, 23, 42, 0.4);
  border-top: 1px solid rgba(255, 255, 255, 0.06);
  flex-wrap: wrap;
  gap: 0.75rem;
}

.pagination-info {
  font-size: 0.78rem;
  color: #94a3b8;
}

.pagination-controls {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.page-numbers {
  display: flex;
  gap: 0.25rem;
}

.page-number-btn {
  background: transparent;
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: #cbd5e1;
  padding: 0.2rem 0.55rem;
  border-radius: 4px;
  font-size: 0.75rem;
  cursor: pointer;
}

.page-number-btn.active {
  background: #2563eb;
  border-color: #2563eb;
  color: #ffffff;
  font-weight: 600;
}

/* Modal Styling */
.modal-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(4px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 100;
  padding: 1.5rem;
}

.modal-card {
  background: #111827;
  border: 1px solid rgba(255, 255, 255, 0.12);
  border-radius: 14px;
  width: 100%;
  max-width: 580px;
  box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  padding: 1.25rem 1.5rem;
  border-bottom: 1px solid rgba(255, 255, 255, 0.08);
}

.modal-title-group {
  display: flex;
  align-items: center;
  gap: 0.65rem;
}

.modal-header h3 {
  margin: 0;
  font-size: 1.1rem;
  color: #ffffff;
}

.modal-subtitle {
  margin: 0.2rem 0 0 0;
  font-size: 0.78rem;
  color: #94a3b8;
}

.modal-close {
  background: transparent;
  border: none;
  font-size: 1.5rem;
  color: #94a3b8;
  cursor: pointer;
  line-height: 1;
}

.modal-close:hover {
  color: #ffffff;
}

.modal-body {
  padding: 1.25rem 1.5rem;
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.meta-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.meta-label {
  font-size: 0.8rem;
  color: #94a3b8;
  margin-right: 0.4rem;
}

.meta-value {
  font-size: 0.82rem;
  color: #e2e8f0;
  font-weight: 500;
}

.json-code-box {
  background: #090d16;
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 8px;
  padding: 1rem;
  max-height: 280px;
  overflow-y: auto;
}

.json-code-box pre {
  margin: 0;
  font-family: 'JetBrains Mono', monospace;
  font-size: 0.8rem;
  color: #38bdf8;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 0.4rem;
}

.form-label {
  font-size: 0.78rem;
  font-weight: 500;
  color: #cbd5e1;
}

.form-input {
  background: #1e293b;
  border: 1px solid rgba(255, 255, 255, 0.12);
  color: #f1f5f9;
  padding: 0.55rem 0.75rem;
  border-radius: 6px;
  font-size: 0.85rem;
  outline: none;
}

.form-input:focus, .form-textarea:focus {
  border-color: #3b82f6;
}

.form-textarea {
  background: #1e293b;
  border: 1px solid rgba(255, 255, 255, 0.12);
  color: #f1f5f9;
  padding: 0.65rem 0.75rem;
  border-radius: 6px;
  font-size: 0.82rem;
  outline: none;
  resize: vertical;
}

.template-buttons {
  display: flex;
  gap: 0.4rem;
  flex-wrap: wrap;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 0.65rem;
  padding: 1rem 1.5rem;
  border-top: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(15, 23, 42, 0.3);
}

/* Responsif Mobile & Layar Sentuh */
@media (max-width: 768px) {
  .navbar {
    padding: 0.85rem 1rem;
    flex-direction: column;
    align-items: stretch;
    gap: 0.85rem;
  }
  .navbar-left {
    justify-content: flex-start;
  }
  .navbar-right {
    justify-content: space-between;
    gap: 0.5rem;
  }
  .status-indicators {
    width: 100%;
    justify-content: space-between;
  }
  .main-content {
    padding: 1rem;
    gap: 1.25rem;
  }
  .stats-grid {
    grid-template-columns: 1fr;
    gap: 0.85rem;
  }
  .section-toolbar {
    flex-direction: column;
    align-items: stretch;
    gap: 0.75rem;
  }
  .toolbar-controls {
    flex-direction: column;
    align-items: stretch;
    gap: 0.75rem;
  }
  .search-form {
    width: 100%;
  }
  .search-input-wrap {
    flex: 1;
  }
  .search-input {
    width: 100%;
  }
  .limit-select-wrap {
    justify-content: space-between;
  }
  .data-table th, .data-table td {
    padding: 0.75rem 0.85rem;
  }
  .pagination-bar {
    flex-direction: column;
    align-items: stretch;
    gap: 0.75rem;
  }
  .pagination-controls {
    justify-content: space-between;
    width: 100%;
  }
  .modal-card {
    max-width: 95vw;
  }
}
</style>
