const express = require("express");
const router = express.Router();

const animalController = require("../controller/animal");

router.get("/", animalController.getAllAnimals);

router.get("/:animalId", animalController.getAnimalById);

router.post("/", animalController.createAnimal);

router.delete("/:animalId", animalController.deleteAnimal);

module.exports = router;
