const Joi = require("joi");

const createAnimal = Joi.object({
  tagId: Joi.string().length(15).required(),
});

module.exports = {
  createAnimal,
};
