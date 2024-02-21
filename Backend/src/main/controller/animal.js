const animalService = require("../service/animal");
const animalSchema = require("./schema/animal");

const getAllAnimals = async (req, res, next) => {
  const animals = await animalService.getAnimals({
    include: {
      weights: true,
    },
    orderBy: {
      id: "desc",
    },
  });
  animals.forEach((animal) => {
    animal.weights.forEach((weight) => {
      delete weight.id;
      delete weight.animalId;
    });
  });
  res.json(animals);
};

const getAnimalById = async (req, res, next) => {
  const animal = await animalService.getAnimal({
    where: { id: Number(req.params.animalId) },
    include: {
      weights: {
        orderBy: {
          id: "desc",
        },
      },
      groups: true,
    },
  });
  if (!animal) res.status(404).send("Animal not found");
  animal.weights.forEach((weight) => {
    delete weight.id;
    delete weight.animalId;
    weight.date = `${weight.date.getDate()}/${weight.date.getMonth() + 1}/${weight.date.getFullYear()}`;
  });
  animal.created = `${animal.created.getDate()}/${animal.created.getMonth() + 1}/${animal.created.getFullYear()}`;
  res.json(animal);
};

const createAnimal = async (req, res, next) => {
  const { error, value } = animalSchema.createAnimal.validate(req.body);
  if (error) return res.status(400).json({ error: error.details[0] });
  const createdAnimal = await animalService.createAnimal({ data: value });
  res.json(createdAnimal);
};

const deleteAnimal = async (req, res, next) => {
  const animal = await animalService.getAnimal({
    where: { id: Number(req.params.animalId) },
  });
  if (!animal) res.status(404).send("Animal not found");
  const deletedAnimal = await animalService.deleteAnimal({ where: { id: Number(req.params.animalId) } });
  res.json(deletedAnimal);
};

module.exports = {
  getAllAnimals,
  getAnimalById,
  createAnimal,
  deleteAnimal,
};
