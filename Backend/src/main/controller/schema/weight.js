const Joi = require("joi");

const createWeight = Joi.object({
  tagId: Joi.string().length(15).required(),
  weight: Joi.number().integer().min(1).max(999).required(),
});

module.exports = {
  createWeight,
};
