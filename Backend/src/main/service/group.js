const { PrismaClient } = require("@prisma/client");
const prisma = new PrismaClient();
const { Group } = prisma;

const getGroups = async (params) => {
  return await Group.findMany(params);
};

const getGroup = async (params) => {
  return await Group.findFirst(params);
};

const createGroup = async (params) => {
  return await Group.create(params);
};

const updateGroup = async (params) => {
  return await Group.update(params);
};

const deleteGroup = async (params) => {
  return await Group.delete(params);
};

module.exports = {
  getGroups,
  getGroup,
  createGroup,
  updateGroup,
  deleteGroup,
};
