const express = require('express');
const router = express.Router();
const dataController = require('../controllers/data.controller');

// Health check
router.get('/health', dataController.healthCheck);

// CRUD data ESP32
router.post('/data', dataController.createData);
router.get('/data', dataController.getAllData);
router.get('/data/:id', dataController.getDataById);

module.exports = router;
