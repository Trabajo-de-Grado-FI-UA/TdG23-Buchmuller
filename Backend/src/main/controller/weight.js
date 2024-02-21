const weightService = require("../service/weight");
const animalService = require("../service/animal");
const weightSchema = require("./schema/weight");

const getAllWeights = async (req, res, next) => {
  const weights = await weightService.getWeights({
    include: {
      animal: true,
    },
    orderBy: {
      id: "desc",
    },
    take: 500,
  });
  weights.forEach((weight) => {
    weight.date = `${weight.date.getDate()}/${weight.date.getMonth() + 1}/${weight.date.getFullYear()}`;
  });
  res.json(weights);
};

const createWeight = async (req, res, next) => {
  const { error, value } = weightSchema.createWeight.validate(req.body);
  if (error) return res.status(400).json({ error: error.details[0] });
  var animal = await animalService.getAnimal({
    where: {
      tagId: value.tagId,
    },
    orderBy: {
      created: "desc",
    },
    include: {
      weights: true,
    },
  });
  if (!animal) {
    animal = await animalService.createAnimal({ data: { tagId: value.tagId }, include: { weights: true } });
  }
  const lastWeight = animal.weights.length > 0 ? animal.weights[animal.weights.length - 1] : null;
  if (
    lastWeight &&
    animal.created < Date.now() - 1000 * 3600 * 24 * 90 &&
    value.weight - lastWeight.weight < value.weight * -0.25
  ) {
    console.log("Animal created due to new lower weight");
    animal = await animalService.createAnimal({ data: { tagId: value.tagId }, include: { weights: true } });
  } else if (lastWeight && Date.now() - lastWeight.date < 1000 * 3600 * 24) {
    const deletedWeight = await weightService.deleteWeight({ where: { id: lastWeight.id } });
    console.log(`Deleted previous weight:`);
    console.log(deletedWeight);
  }
  const createdWeight = await weightService.createWeight({
    data: {
      animalId: animal.id,
      weight: value.weight,
    },
  });
  res.json(createdWeight);
};

module.exports = {
  getAllWeights,
  createWeight,
};
