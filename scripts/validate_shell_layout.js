#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const filePath = process.argv[2] || path.join(__dirname, "..", "data", "shell_layout.json");
const document = JSON.parse(fs.readFileSync(filePath, "utf8"));
const panels = document.panels || {};

function assertPanel(name, sizeKey, min, max) {
  const panel = panels[name];
  if (!panel || typeof panel !== "object") {
    throw new Error(`panels.${name} must be an object`);
  }
  if (typeof panel.collapsed !== "boolean") {
    throw new Error(`panels.${name}.collapsed must be boolean`);
  }
  if (!Number.isInteger(panel[sizeKey]) || panel[sizeKey] < min || panel[sizeKey] > max) {
    throw new Error(`panels.${name}.${sizeKey} must be integer ${min}-${max}`);
  }
}

assertPanel("left", "width", 180, 520);
assertPanel("right", "width", 240, 460);
assertPanel("bottom", "height", 96, 360);

console.log(`${filePath}: ok`);
