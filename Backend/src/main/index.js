require("dotenv").config();
require("express-async-errors");
const { API_PORT } = process.env;
const port = process.env.PORT || API_PORT;
const express = require("express");
const app = express();

app.use(express.json());

const error = require("./middleware/error");
const animalRoutes = require("./route/animal");
const groupRoutes = require("./route/group");
const weightRoutes = require("./route/weight");

process.on("uncaughtException", (ex) => {
  console.error(ex);
});

app.get("/", async (req, res, next) => {
  res.send("Tesis app online");
});

app.use("/animal", animalRoutes);
app.use("/group", groupRoutes);
app.use("/weight", weightRoutes);

app.use(error);

app.listen(port, () => {
  console.log(`App started on port ${port}`);
});
