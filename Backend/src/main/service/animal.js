const { PrismaClient } = require("@prisma/client");
const prisma = new PrismaClient();
const { Animal } = prisma;

const getAnimals = async (params) => {
  return await Animal.findMany(params);
};

const getAnimal = async (params) => {
  return await Animal.findFirst(params);
};

const createAnimal = async (params) => {
  return await Animal.create(params);
};

const updateAnimal = async (params) => {
  return await Animal.update(params);
};

const deleteAnimal = async (params) => {
  return await Animal.delete(params);
};

module.exports = {
  getAnimals,
  getAnimal,
  createAnimal,
  updateAnimal,
  deleteAnimal,
};
