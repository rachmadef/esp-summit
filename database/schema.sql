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
