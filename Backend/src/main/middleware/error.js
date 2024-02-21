module.exports = function (err, req, res, next) {
  const code = err.code;
  console.error(err, { url: req.originalUrl });
  switch (code) {
    case "P2002":
      res.status(409).send("Resource already exists");
      break;
    case "P2025":
      res.status(400).send("Resource not found");
      break;
    default:
      res.status(500).send(err);
      break;
  }
};
