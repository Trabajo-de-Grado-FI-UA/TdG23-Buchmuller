const Joi = require("joi");

const createGroup = Joi.object({
  name: Joi.string().required(),
});

const addAnimalsToGroup = Joi.object({
  tagIds: Joi.array().items(Joi.string().length(15)).required(),
});

module.exports = {
  createGroup,
  addAnimalsToGroup,
};
