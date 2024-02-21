const groupService = require("../service/group");
const animalService = require("../service/animal");
const groupSchema = require("./schema/group");

const getAllGroups = async (req, res, next) => {
  const groups = await groupService.getGroups({
    include: {
      animals: true,
    },
    orderBy: {
      id: "desc",
    },
  });
  groups.forEach((group) => {
    group.animalsCount = group.animals.length;
    delete group.animals;
  });
  res.json(groups);
};

const getGroupById = async (req, res, next) => {
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
    include: {
      animals: {
        include: {
          weights: true,
        },
      },
    },
  });
  if (!group) res.status(404).send("Group not found");

  var lastWeights = [],
    lastWeightsPercentile = [],
    accumulatedLastWeights = 0,
    totalWeights = 0,
    maxWeight = 0,
    minWeight = 1000,
    sumOfSquareDif = 0;

  group.animals.forEach((animal) => {
    const lastWeight = animal.weights.length > 0 ? animal.weights[animal.weights.length - 1] : null;
    delete animal.weights;
    if (lastWeight) {
      delete lastWeight.id;
      lastWeights = [...lastWeights, lastWeight];
      lastWeightsPercentile = [...lastWeightsPercentile, lastWeight.weight];
      accumulatedLastWeights += lastWeight.weight;
      totalWeights++;
      if (lastWeight.weight < minWeight) minWeight = lastWeight.weight;
      if (lastWeight.weight > maxWeight) maxWeight = lastWeight.weight;
    }
  });
  if (minWeight == 1000) minWeight = 0;
  const mean = accumulatedLastWeights / totalWeights;
  lastWeightsPercentile.sort((a, b) => a - b);
  lastWeights.sort((a, b) => new Date(b.date) - new Date(a.date));
  lastWeights.forEach((weight) => {
    weight.date = `${weight.date.getDate()}/${weight.date.getMonth() + 1}/${weight.date.getFullYear()}`;
    const diff = weight.weight - mean;
    sumOfSquareDif += diff * diff;
  });
  const variance = sumOfSquareDif / totalWeights;
  const standardDeviation = Math.sqrt(variance);

  const percentile25 = calculatePercentile(lastWeightsPercentile, 25);
  const percentile50 = calculatePercentile(lastWeightsPercentile, 50);
  const percentile75 = calculatePercentile(lastWeightsPercentile, 75);

  group.weights = lastWeights;
  group.maxWeight = parseFloat(maxWeight.toFixed(1));
  group.minWeight = parseFloat(minWeight.toFixed(1));
  group.totalWeights = totalWeights;
  group.middleWeight = parseFloat(mean.toFixed(1));
  group.variance = parseFloat(variance.toFixed(1));
  group.standardDeviation = parseFloat(standardDeviation.toFixed(1));
  group.percentile25 = parseFloat(percentile25.toFixed(1));
  group.percentile50 = parseFloat(percentile50.toFixed(1));
  group.percentile75 = parseFloat(percentile75.toFixed(1));
  if (group) res.json(group);
};

function calculatePercentile(data, percentile) {
  const index = (percentile / 100) * (data.length - 1);
  if (Number.isInteger(index)) {
    return data[index];
  } else {
    const floor = Math.floor(index);
    const fraction = index - floor;
    return data[floor] + fraction * (data[floor + 1] - data[floor]);
  }
}

const createGroup = async (req, res, next) => {
  const { error, value } = groupSchema.createGroup.validate(req.body);
  if (error) return res.status(400).json({ error: error.details[0] });
  const createdGroup = await groupService.createGroup({ data: value });
  res.json(createdGroup);
};

const addAnimalToGroup = async (req, res, next) => {
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
  });
  if (!group) return res.status(400).send("Group does not exist");

  const animal = await animalService.getAnimal({
    where: {
      tagId: req.params.tagId ? String(req.params.tagId) : undefined,
      id: req.params.animalId ? Number(req.params.animalId) : undefined,
    },
    orderBy: {
      created: "desc",
    },
  });
  if (!animal) return res.status(400).send("Animal does not exist");

  const updatedGroup = await groupService.updateGroup({
    where: { id: group.id },
    data: {
      animals: { connect: { id: animal.id } },
    },
    include: {
      animals: true,
    },
  });

  res.json(updatedGroup);
};

const addAnimalsToGroup = async (req, res, next) => {
  const { error, value } = groupSchema.addAnimalsToGroup.validate(req.body);
  if (error) return res.status(400).json({ error: error.details[0] });
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
  });
  if (!group) return res.status(400).send("Group does not exist");

  var updatedGroup;
  for (const tagId of value.tagIds) {
    const animal = await animalService.getAnimal({
      where: { tagId },
      orderBy: {
        created: "desc",
      },
    });
    console.log(animal);

    updatedGroup = await groupService.updateGroup({
      where: { id: group.id },
      data: {
        animals: { connect: { id: animal.id } },
      },
      include: {
        animals: true,
      },
    });
  }

  res.json(updatedGroup);
};

const removeAnimalFromGroup = async (req, res, next) => {
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
    include: {
      animals: true,
    },
  });
  if (!group) return res.status(400).send("Group does not exist");
  if (
    group.animals.filter((animal) =>
      req.params.tagId ? animal.tagId == String(req.params.tagId) : animal.id == Number(req.params.animalId)
    ).length == 0
  )
    return res.status(400).send("Animal does not exist in group");

  const animal = await animalService.getAnimal({
    where: {
      tagId: req.params.tagId ? String(req.params.tagId) : undefined,
      id: req.params.animalId ? Number(req.params.animalId) : undefined,
    },
    orderBy: {
      created: "desc",
    },
  });
  if (!animal) return res.status(400).send("Animal does not exist");

  const updatedGroup = await groupService.updateGroup({
    where: { id: group.id },
    data: {
      animals: { disconnect: { id: animal.id } },
    },
    include: {
      animals: true,
    },
  });

  res.json(updatedGroup);
};

const deleteGroup = async (req, res, next) => {
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
    include: {
      animals: true,
    },
  });
  if (!group) return res.status(400).send("Group does not exist");
  const deletedGroup = await groupService.deleteGroup({ where: { id: Number(req.params.groupId) } });
  res.json(deletedGroup);
};

const changeGroupName = async (req, res, next) => {
  if (!req.body.name) return res.status(400).send("Invalid group name");
  const group = await groupService.getGroup({
    where: { id: Number(req.params.groupId) },
    include: {
      animals: true,
    },
  });
  if (!group) return res.status(400).send("Group does not exist");
  const updatedGroup = await groupService.updateGroup({
    where: { id: Number(req.params.groupId) },
    data: { name: req.body.name },
  });
  res.json(updatedGroup);
};

module.exports = {
  getAllGroups,
  getGroupById,
  createGroup,
  addAnimalToGroup,
  removeAnimalFromGroup,
  addAnimalsToGroup,
  deleteGroup,
  changeGroupName,
};
