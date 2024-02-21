const express = require("express");
const router = express.Router();

const weightController = require("../controller/weight");

router.get("/", weightController.getAllWeights);

router.post("/", weightController.createWeight);

module.exports = router;
