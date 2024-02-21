const { PrismaClient } = require("@prisma/client");
const prisma = new PrismaClient();
const { Weight } = prisma;

const getWeights = async (params) => {
  return await Weight.findMany(params);
};

const createWeight = async (params) => {
  return await Weight.create(params);
};

const updateWeight = async (params) => {
  return await Weight.update(params);
};

const deleteWeight = async (params) => {
  return await Weight.delete(params);
};

module.exports = {
  getWeights,
  createWeight,
  updateWeight,
  deleteWeight,
};
