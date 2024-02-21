const express = require("express");
const router = express.Router();

const groupController = require("../controller/group");

router.get("/", groupController.getAllGroups);

router.get("/:groupId", groupController.getGroupById);

router.post("/", groupController.createGroup);

router.post("/:groupId/animal/:animalId", groupController.addAnimalToGroup);

router.post("/:groupId/tag/:tagId", groupController.addAnimalToGroup);

router.post("/:groupId", groupController.addAnimalsToGroup);

router.patch("/:groupId", groupController.changeGroupName);

router.delete("/:groupId/animal/:animalId", groupController.removeAnimalFromGroup);

router.delete("/:groupId/tag/:tagId", groupController.removeAnimalFromGroup);

router.delete("/:groupId", groupController.deleteGroup);

module.exports = router;
