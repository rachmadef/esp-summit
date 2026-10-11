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

// Auto Refresh Timer (disesuaikan dengan interval ESP32: 3 detik)
const refreshIntervalSec = ref(3);
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

// Status salin JSON
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
  <div class="app-container">
    <!-- Top Navbar -->
    <header class="top-nav">
      <div class="nav-content">
        <div class="nav-brand">
          <div class="brand-icon">
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"></path>
            </svg>
          </div>
          <div>
            <h1 class="brand-title">ESP32 Monitoring</h1>
            <p class="brand-sub">Laragon &bull; Express &bull; Vue 3</p>
          </div>
        </div>

        <div class="nav-actions">
          <!-- Connection Status Pills -->
          <div class="status-group">
            <span class="status-pill" :class="healthStatus.status === 'healthy' ? 'pill-success' : 'pill-danger'">
              <span class="status-dot"></span>
              API: {{ healthStatus.status === 'healthy' ? 'Online' : 'Offline' }}
            </span>

            <span class="status-pill" :class="healthStatus.database === 'connected' ? 'pill-success' : 'pill-danger'">
              <span class="status-dot"></span>
              DB: {{ healthStatus.database === 'connected' ? 'Terhubung' : 'Terputus' }}
            </span>
          </div>

          <!-- Auto Refresh Selector -->
          <div class="action-item">
            <label for="auto-refresh-select" class="form-label-inline">Auto-refresh:</label>
            <select id="auto-refresh-select" v-model="refreshIntervalSec" class="form-select">
              <option :value="0">Mati</option>
              <option :value="3">3 Detik</option>
              <option :value="5">5 Detik</option>
              <option :value="10">10 Detik</option>
              <option :value="30">30 Detik</option>
            </select>
          </div>

          <!-- Refresh Button -->
          <button id="btn-refresh" class="btn btn-default" @click="refreshAll" :disabled="isLoadingData" title="Segarkan Data">
            <svg :class="{ 'spin-anim': isLoadingData }" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <polyline points="23 4 23 10 17 10"></polyline>
              <polyline points="1 20 1 14 7 14"></polyline>
              <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path>
            </svg>
            Segarkan
          </button>

          <!-- Simulator Trigger Button -->
          <button id="btn-open-simulator" class="btn btn-primary" @click="showSimulatorModal = true">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <polygon points="5 3 19 12 5 21 5 3"></polygon>
            </svg>
            Simulator ESP32
          </button>
        </div>
      </div>
    </header>

    <!-- Main Content Area -->
    <main class="page-body">
      <!-- Error Alert -->
      <div v-if="errorMessage" class="notice notice-danger">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="10"></circle>
          <line x1="12" y1="8" x2="12" y2="12"></line>
          <line x1="12" y1="16" x2="12.01" y2="16"></line>
        </svg>
        <span>{{ errorMessage }}</span>
        <button class="notice-dismiss" @click="errorMessage = ''">&times;</button>
      </div>

      <!-- Stat Cards Grid -->
      <section class="cards-grid">
        <div class="metric-card">
          <div class="metric-header">
            <span class="metric-label">Total Data Masuk</span>
            <span class="metric-icon icon-blue">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <ellipse cx="12" cy="5" rx="9" ry="3"></ellipse>
                <path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3"></path>
                <path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5"></path>
              </svg>
            </span>
          </div>
          <div class="metric-value">{{ totalRecords }}</div>
          <div class="metric-caption">Tersimpan di database lokal</div>
        </div>

        <div class="metric-card">
          <div class="metric-header">
            <span class="metric-label">Perangkat Aktif</span>
            <span class="metric-icon icon-green">
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
          <div class="metric-value">{{ uniqueDevicesCount }}</div>
          <div class="metric-caption">ID perangkat unik di tabel</div>
        </div>

        <div class="metric-card">
          <div class="metric-header">
            <span class="metric-label">Data Terakhir</span>
            <span class="metric-icon icon-purple">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"></circle>
                <polyline points="12 6 12 12 16 14"></polyline>
              </svg>
            </span>
          </div>
          <div class="metric-value metric-value-sm">{{ latestRecordTime }}</div>
          <div class="metric-caption">Waktu diterima server</div>
        </div>

        <div class="metric-card">
          <div class="metric-header">
            <span class="metric-label">Info Endpoint ESP32</span>
            <span class="metric-icon icon-amber">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
                <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
                <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
                <line x1="12" y1="20" x2="12.01" y2="20"></line>
              </svg>
            </span>
          </div>
          <div class="metric-value metric-value-sm font-mono">:3000/api/data</div>
          <div class="metric-caption">Gunakan IP Komputer di Wi-Fi</div>
        </div>
      </section>

      <!-- Data Table Card -->
      <section class="panel">
        <!-- Panel Toolbar -->
        <div class="panel-header">
          <div class="panel-title-area">
            <h2 class="panel-title">Riwayat Telemetri Data</h2>
            <span class="panel-badge">Halaman {{ currentPage }} dari {{ totalPages }}</span>
          </div>

          <div class="panel-actions">
            <!-- Filter by Device ID -->
            <form class="filter-form" @submit.prevent="handleFilterSubmit">
              <div class="input-with-icon">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <circle cx="11" cy="11" r="8"></circle>
                  <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
                </svg>
                <input
                  v-model="filterDeviceId"
                  type="text"
                  placeholder="Cari Device ID..."
                  class="form-input"
                />
              </div>
              <button type="submit" class="btn btn-default btn-sm">Cari</button>
              <button v-if="filterDeviceId" type="button" class="btn btn-link btn-sm" @click="resetFilter">Reset</button>
            </form>

            <!-- Limit selector -->
            <div class="limit-wrapper">
              <label for="limit-select" class="form-label-inline">Limit:</label>
              <select id="limit-select" v-model="limitPerPage" @change="loadData" class="form-select form-select-sm">
                <option :value="10">10</option>
                <option :value="20">20</option>
                <option :value="50">50</option>
              </select>
            </div>
          </div>
        </div>

        <!-- Table View -->
        <div class="panel-body">
          <div v-if="isLoadingData" class="state-box">
            <div class="spinner"></div>
            <p>Memuat data...</p>
          </div>

          <div v-else-if="dataList.length === 0" class="state-box">
            <svg width="44" height="44" viewBox="0 0 24 24" fill="none" stroke="#94a3b8" stroke-width="1.5">
              <circle cx="12" cy="12" r="10"></circle>
              <line x1="8" y1="12" x2="16" y2="12"></line>
            </svg>
            <h3>Belum Ada Data</h3>
            <p>Tabel masih kosong. Tunggu ESP32 mengirim data atau gunakan simulator untuk uji coba.</p>
            <button class="btn btn-primary btn-sm" @click="showSimulatorModal = true">Kirim Data Uji Coba</button>
          </div>

          <div v-else class="table-container">
            <table class="clean-table">
              <thead>
                <tr>
                  <th style="width: 70px;">ID</th>
                  <th style="width: 170px;">Device ID</th>
                  <th>Payload Data (JSON)</th>
                  <th style="width: 210px;">Waktu Diterima</th>
                  <th style="width: 90px; text-align: right;">Aksi</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="item in dataList" :key="item.id">
                  <td class="font-mono text-muted">#{{ item.id }}</td>
                  <td>
                    <span class="device-tag">
                      <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <circle cx="12" cy="12" r="2"></circle>
                        <path d="M16.24 7.76a6 6 0 0 1 0 8.49m-8.48-.01a6 6 0 0 1 0-8.49m11.31-2.82a10 10 0 0 1 0 14.14m-14.14 0a10 10 0 0 1 0-14.14"></path>
                      </svg>
                      {{ item.device_id }}
                    </span>
                  </td>
                  <td>
                    <div class="json-tags-row">
                      <span v-for="(val, key) in item.data_json" :key="key" class="json-chip">
                        <span class="chip-key">{{ key }}:</span>
                        <span class="chip-val">{{ typeof val === 'object' ? JSON.stringify(val) : val }}</span>
                      </span>
                    </div>
                  </td>
                  <td class="text-muted text-sm">
                    {{ formatDate(item.received_at) }}
                  </td>
                  <td style="text-align: right;">
                    <button class="btn btn-link btn-sm" @click="openDetail(item)">
                      Detail
                    </button>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>

          <!-- Pagination Footer -->
          <div v-if="totalPages > 1" class="pagination-footer">
            <span class="pagination-text">
              Menampilkan {{ dataList.length }} dari {{ totalRecords }} rekaman
            </span>
            <div class="pagination-buttons">
              <button
                class="btn btn-default btn-xs"
                :disabled="currentPage <= 1"
                @click="goToPage(currentPage - 1)"
              >
                &larr; Sebelumnya
              </button>

              <div class="page-pills">
                <button
                  v-for="p in totalPages"
                  :key="p"
                  class="page-pill"
                  :class="{ active: p === currentPage }"
                  @click="goToPage(p)"
                >
                  {{ p }}
                </button>
              </div>

              <button
                class="btn btn-default btn-xs"
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
    <div v-if="showDetailModal && selectedRecord" class="modal-overlay" @click.self="closeDetail">
      <div class="modal-box">
        <div class="modal-box-header">
          <div class="modal-title-wrap">
            <h3 class="modal-title">Detail Data #{{ selectedRecord.id }}</h3>
            <span class="device-tag">{{ selectedRecord.device_id }}</span>
          </div>
          <button class="modal-close-btn" @click="closeDetail">&times;</button>
        </div>

        <div class="modal-box-body">
          <div class="modal-meta-row">
            <span class="text-muted text-sm">Waktu: {{ formatDate(selectedRecord.received_at) }}</span>
            <button class="btn btn-default btn-xs" @click="copyJsonToClipboard(selectedRecord.data_json)">
              <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect>
                <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path>
              </svg>
              {{ isCopied ? 'Tersalin' : 'Salin JSON' }}
            </button>
          </div>

          <div class="code-view">
            <pre><code>{{ JSON.stringify(selectedRecord.data_json, null, 2) }}</code></pre>
          </div>
        </div>

        <div class="modal-box-footer">
          <button class="btn btn-default" @click="closeDetail">Tutup</button>
        </div>
      </div>
    </div>

    <!-- Modal Simulator ESP32 -->
    <div v-if="showSimulatorModal" class="modal-overlay" @click.self="showSimulatorModal = false">
      <div class="modal-box">
        <div class="modal-box-header">
          <div>
            <h3 class="modal-title">Simulator ESP32</h3>
            <p class="modal-desc">Kirim payload JSON untuk menguji penerimaan data</p>
          </div>
          <button class="modal-close-btn" @click="showSimulatorModal = false">&times;</button>
        </div>

        <div class="modal-box-body">
          <!-- Template selector -->
          <div class="form-row">
            <label class="form-label">Template Contoh:</label>
            <div class="template-chips">
              <button type="button" class="btn btn-default btn-xs" @click="setSimulatorTemplate('suhu')">Suhu & Kelembaban</button>
              <button type="button" class="btn btn-default btn-xs" @click="setSimulatorTemplate('tegangan')">Baterai & Tegangan</button>
              <button type="button" class="btn btn-default btn-xs" @click="setSimulatorTemplate('gerakan')">Sensor PIR</button>
            </div>
          </div>

          <!-- Device ID -->
          <div class="form-row">
            <label for="sim-device" class="form-label">Device ID:</label>
            <input
              id="sim-device"
              v-model="simulatorDeviceId"
              type="text"
              class="form-input"
              placeholder="Contoh: esp32-ruang-utama"
            />
          </div>

          <!-- JSON Payload -->
          <div class="form-row">
            <label for="sim-data" class="form-label">Data JSON (Objek):</label>
            <textarea
              id="sim-data"
              v-model="simulatorPayloadText"
              rows="6"
              class="form-textarea font-mono"
            ></textarea>
          </div>

          <!-- Feedback message -->
          <div v-if="simulatorStatus.message" class="notice" :class="simulatorStatus.success ? 'notice-success' : 'notice-danger'">
            {{ simulatorStatus.message }}
          </div>
        </div>

        <div class="modal-box-footer">
          <button class="btn btn-default" @click="showSimulatorModal = false">Batal</button>
          <button
            class="btn btn-primary"
            :disabled="simulatorStatus.loading || !simulatorDeviceId"
            @click="submitSimulator"
          >
            <svg v-if="simulatorStatus.loading" class="spin-anim" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <line x1="12" y1="2" x2="12" y2="6"></line>
              <line x1="12" y1="18" x2="12" y2="22"></line>
            </svg>
            {{ simulatorStatus.loading ? 'Mengirim...' : 'Kirim Data' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* -------------------------------------------------------------
   LIGHT MODE DESIGN SYSTEM - CLEAN & MINIMALIST
------------------------------------------------------------- */
.app-container {
  min-height: 100vh;
  background-color: #f8fafc; /* Slate 50 */
  color: #0f172a;            /* Slate 900 */
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  -webkit-font-smoothing: antialiased;
}

/* Navbar */
.top-nav {
  background-color: #ffffff;
  border-bottom: 1px solid #e2e8f0;
  position: sticky;
  top: 0;
  z-index: 40;
}

.nav-content {
  max-width: 1280px;
  margin: 0 auto;
  padding: 0.85rem 1.5rem;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 1rem;
}

.nav-brand {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.brand-icon {
  width: 36px;
  height: 36px;
  background-color: #2563eb;
  color: #ffffff;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.brand-title {
  font-size: 1.1rem;
  font-weight: 600;
  color: #0f172a;
  margin: 0;
  line-height: 1.2;
}

.brand-sub {
  font-size: 0.75rem;
  color: #64748b;
  margin: 0.15rem 0 0 0;
}

.nav-actions {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

/* Status Pills */
.status-group {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.status-pill {
  display: inline-flex;
  align-items: center;
  gap: 0.35rem;
  padding: 0.25rem 0.6rem;
  border-radius: 9999px;
  font-size: 0.75rem;
  font-weight: 500;
}

.pill-success {
  background-color: #ecfdf5;
  color: #059669;
  border: 1px solid #a7f3d0;
}

.pill-danger {
  background-color: #fef2f2;
  color: #dc2626;
  border: 1px solid #fecaca;
}

.status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background-color: currentColor;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

.form-label-inline {
  font-size: 0.8rem;
  color: #64748b;
}

/* Form Controls */
.form-select {
  background-color: #ffffff;
  border: 1px solid #cbd5e1;
  color: #334155;
  padding: 0.4rem 0.6rem;
  border-radius: 6px;
  font-size: 0.82rem;
  outline: none;
  cursor: pointer;
  transition: border-color 0.15s ease;
}

.form-select:focus {
  border-color: #2563eb;
}

.form-select-sm {
  padding: 0.25rem 0.5rem;
  font-size: 0.78rem;
}

.form-input {
  background-color: #ffffff;
  border: 1px solid #cbd5e1;
  color: #0f172a;
  padding: 0.4rem 0.65rem;
  border-radius: 6px;
  font-size: 0.82rem;
  outline: none;
  transition: border-color 0.15s ease;
}

.form-input:focus, .form-textarea:focus {
  border-color: #2563eb;
}

.form-textarea {
  background-color: #ffffff;
  border: 1px solid #cbd5e1;
  color: #0f172a;
  padding: 0.6rem 0.75rem;
  border-radius: 6px;
  font-size: 0.82rem;
  outline: none;
  width: 100%;
  box-sizing: border-box;
}

/* Buttons */
.btn {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.45rem 0.8rem;
  border-radius: 6px;
  font-size: 0.82rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.15s ease;
  border: 1px solid transparent;
}

.btn-default {
  background-color: #ffffff;
  color: #334155;
  border-color: #cbd5e1;
}

.btn-default:hover:not(:disabled) {
  background-color: #f1f5f9;
  border-color: #94a3b8;
}

.btn-primary {
  background-color: #2563eb;
  color: #ffffff;
}

.btn-primary:hover:not(:disabled) {
  background-color: #1d4ed8;
}

.btn-link {
  background: transparent;
  color: #2563eb;
  border: none;
  padding: 0.2rem 0.4rem;
}

.btn-link:hover {
  text-decoration: underline;
}

.btn-sm {
  padding: 0.35rem 0.65rem;
  font-size: 0.78rem;
}

.btn-xs {
  padding: 0.25rem 0.5rem;
  font-size: 0.75rem;
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

/* Page Body */
.page-body {
  max-width: 1280px;
  margin: 0 auto;
  padding: 1.5rem;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

/* Notice / Alert */
.notice {
  display: flex;
  align-items: center;
  gap: 0.65rem;
  padding: 0.75rem 1rem;
  border-radius: 8px;
  font-size: 0.85rem;
}

.notice-danger {
  background-color: #fef2f2;
  border: 1px solid #fecaca;
  color: #b91c1c;
}

.notice-success {
  background-color: #ecfdf5;
  border: 1px solid #a7f3d0;
  color: #047857;
}

.notice-dismiss {
  margin-left: auto;
  background: none;
  border: none;
  color: inherit;
  font-size: 1.25rem;
  cursor: pointer;
}

/* Cards Grid */
.cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 1rem;
}

.metric-card {
  background-color: #ffffff;
  border: 1px solid #e2e8f0;
  border-radius: 10px;
  padding: 1.15rem;
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.03);
}

.metric-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.metric-label {
  font-size: 0.8rem;
  color: #64748b;
  font-weight: 500;
}

.metric-icon {
  width: 32px;
  height: 32px;
  border-radius: 6px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.icon-blue { background-color: #eff6ff; color: #2563eb; }
.icon-green { background-color: #f0fdf4; color: #16a34a; }
.icon-purple { background-color: #faf5ff; color: #7c3aed; }
.icon-amber { background-color: #fffbeb; color: #d97706; }

.metric-value {
  font-size: 1.75rem;
  font-weight: 700;
  color: #0f172a;
}

.metric-value-sm {
  font-size: 0.95rem;
  font-weight: 600;
}

.metric-caption {
  font-size: 0.75rem;
  color: #94a3b8;
}

/* Panel */
.panel {
  background-color: #ffffff;
  border: 1px solid #e2e8f0;
  border-radius: 10px;
  box-shadow: 0 1px 3px 0 rgba(0, 0, 0, 0.03);
  overflow: hidden;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1rem 1.25rem;
  border-bottom: 1px solid #e2e8f0;
  flex-wrap: wrap;
  gap: 0.85rem;
}

.panel-title-area {
  display: flex;
  align-items: center;
  gap: 0.65rem;
}

.panel-title {
  font-size: 1rem;
  font-weight: 600;
  color: #0f172a;
  margin: 0;
}

.panel-badge {
  font-size: 0.75rem;
  background-color: #f1f5f9;
  color: #64748b;
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
}

.panel-actions {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.filter-form {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.input-with-icon {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  background-color: #ffffff;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  padding: 0.25rem 0.5rem;
  color: #94a3b8;
}

.input-with-icon .form-input {
  border: none;
  padding: 0.2rem;
  outline: none;
  width: 150px;
}

.limit-wrapper {
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

/* Table */
.table-container {
  overflow-x: auto;
}

.clean-table {
  width: 100%;
  border-collapse: collapse;
  text-align: left;
  font-size: 0.84rem;
}

.clean-table th {
  background-color: #f8fafc;
  color: #475569;
  font-weight: 600;
  font-size: 0.74rem;
  text-transform: uppercase;
  letter-spacing: 0.03em;
  padding: 0.75rem 1.25rem;
  border-bottom: 1px solid #e2e8f0;
}

.clean-table td {
  padding: 0.85rem 1.25rem;
  border-bottom: 1px solid #f1f5f9;
  color: #1e293b;
}

.clean-table tr:hover td {
  background-color: #f8fafc;
}

.device-tag {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  background-color: #eff6ff;
  color: #1d4ed8;
  border: 1px solid #dbeafe;
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 0.78rem;
  font-weight: 500;
}

.json-tags-row {
  display: flex;
  flex-wrap: wrap;
  gap: 0.35rem;
}

.json-chip {
  background-color: #f1f5f9;
  border: 1px solid #e2e8f0;
  padding: 0.15rem 0.45rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-family: 'JetBrains Mono', monospace;
  color: #334155;
}

.chip-key {
  color: #0369a1;
  font-weight: 600;
  margin-right: 0.2rem;
}

.chip-val {
  color: #334155;
}

.font-mono {
  font-family: 'JetBrains Mono', monospace;
}

.text-muted {
  color: #64748b;
}

.text-sm {
  font-size: 0.8rem;
}

/* State Box (Empty & Loading) */
.state-box {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 3.5rem 1.5rem;
  text-align: center;
  color: #64748b;
  gap: 0.5rem;
}

.state-box h3 {
  color: #0f172a;
  margin: 0.4rem 0 0 0;
  font-size: 1rem;
}

.state-box p {
  font-size: 0.85rem;
  margin: 0 0 0.75rem 0;
  max-width: 400px;
}

.spinner {
  width: 26px;
  height: 26px;
  border: 2px solid #e2e8f0;
  border-top-color: #2563eb;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

.spin-anim {
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  100% { transform: rotate(360deg); }
}

/* Pagination */
.pagination-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75rem 1.25rem;
  background-color: #ffffff;
  border-top: 1px solid #e2e8f0;
  flex-wrap: wrap;
  gap: 0.65rem;
}

.pagination-text {
  font-size: 0.78rem;
  color: #64748b;
}

.pagination-buttons {
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

.page-pills {
  display: flex;
  gap: 0.2rem;
}

.page-pill {
  background-color: #ffffff;
  border: 1px solid #e2e8f0;
  color: #475569;
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
  font-size: 0.75rem;
  cursor: pointer;
}

.page-pill.active {
  background-color: #2563eb;
  border-color: #2563eb;
  color: #ffffff;
  font-weight: 600;
}

/* Modal */
.modal-overlay {
  position: fixed;
  inset: 0;
  background-color: rgba(15, 23, 42, 0.45);
  backdrop-filter: blur(2px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 100;
  padding: 1rem;
}

.modal-box {
  background-color: #ffffff;
  border: 1px solid #e2e8f0;
  border-radius: 12px;
  width: 100%;
  max-width: 520px;
  box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 8px 10px -6px rgba(0, 0, 0, 0.1);
  display: flex;
  flex-direction: column;
}

.modal-box-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  padding: 1rem 1.25rem;
  border-bottom: 1px solid #e2e8f0;
}

.modal-title-wrap {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.modal-title {
  margin: 0;
  font-size: 1.05rem;
  color: #0f172a;
  font-weight: 600;
}

.modal-desc {
  margin: 0.2rem 0 0 0;
  font-size: 0.78rem;
  color: #64748b;
}

.modal-close-btn {
  background: none;
  border: none;
  font-size: 1.4rem;
  color: #94a3b8;
  cursor: pointer;
  line-height: 1;
}

.modal-close-btn:hover {
  color: #0f172a;
}

.modal-box-body {
  padding: 1.15rem 1.25rem;
  display: flex;
  flex-direction: column;
  gap: 0.85rem;
}

.modal-meta-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.code-view {
  background-color: #f8fafc;
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  padding: 0.85rem;
  max-height: 240px;
  overflow-y: auto;
}

.code-view pre {
  margin: 0;
  font-family: 'JetBrains Mono', monospace;
  font-size: 0.8rem;
  color: #0f172a;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
}

.form-label {
  font-size: 0.8rem;
  font-weight: 500;
  color: #334155;
}

.template-chips {
  display: flex;
  gap: 0.35rem;
  flex-wrap: wrap;
}

.modal-box-footer {
  display: flex;
  justify-content: flex-end;
  gap: 0.5rem;
  padding: 0.85rem 1.25rem;
  border-top: 1px solid #e2e8f0;
  background-color: #f8fafc;
  border-bottom-left-radius: 12px;
  border-bottom-right-radius: 12px;
}

/* Mobile Responsiveness */
@media (max-width: 768px) {
  .nav-content {
    flex-direction: column;
    align-items: stretch;
    padding: 0.75rem 1rem;
  }
  .nav-actions {
    justify-content: space-between;
  }
  .status-group {
    width: 100%;
    justify-content: space-between;
  }
  .page-body {
    padding: 1rem;
    gap: 1rem;
  }
  .cards-grid {
    grid-template-columns: 1fr;
  }
  .panel-header {
    flex-direction: column;
    align-items: stretch;
  }
  .panel-actions {
    flex-direction: column;
    align-items: stretch;
  }
  .filter-form {
    width: 100%;
  }
  .input-with-icon {
    flex: 1;
  }
  .input-with-icon .form-input {
    width: 100%;
  }
  .pagination-footer {
    flex-direction: column;
    align-items: stretch;
  }
  .pagination-buttons {
    justify-content: space-between;
  }
  .modal-box {
    max-width: 95vw;
  }
}
</style>
