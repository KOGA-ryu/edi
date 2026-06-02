#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const filePath = process.argv[2] || path.join(__dirname, "..", "data", "shell_layout.json");
const document = JSON.parse(fs.readFileSync(filePath, "utf8"));
const windowSettings = document.window || {};
const policy = document.policy || {};
const rightPanel = document.right_panel || {};
const panels = document.panels || {};

function assertInteger(pathName, value, min, max) {
  if (!Number.isInteger(value) || value < min || value > max) {
    throw new Error(`${pathName} must be integer ${min}-${max}`);
  }
}

function assertWindow() {
  if (!windowSettings || typeof windowSettings !== "object") {
    throw new Error("window must be an object");
  }
  assertInteger("window.width", windowSettings.width, 520, 2400);
  assertInteger("window.height", windowSettings.height, 420, 1800);
}

function assertPolicy(name, sizeStem, thresholdKey, minLimit, maxLimit, thresholdMax) {
  const panelPolicy = policy[name];
  if (!panelPolicy || typeof panelPolicy !== "object") {
    throw new Error(`policy.${name} must be an object`);
  }

  const minKey = `min_${sizeStem}`;
  const maxKey = `max_${sizeStem}`;
  assertInteger(`policy.${name}.${minKey}`, panelPolicy[minKey], minLimit, maxLimit);
  assertInteger(`policy.${name}.${maxKey}`, panelPolicy[maxKey], panelPolicy[minKey], maxLimit);
  assertInteger(`policy.${name}.${thresholdKey}`, panelPolicy[thresholdKey], 0, thresholdMax);
  return panelPolicy;
}

function assertPanel(name, sizeKey, min, max) {
  const panel = panels[name];
  if (!panel || typeof panel !== "object") {
    throw new Error(`panels.${name} must be an object`);
  }
  if (typeof panel.collapsed !== "boolean") {
    throw new Error(`panels.${name}.collapsed must be boolean`);
  }
  assertInteger(`panels.${name}.${sizeKey}`, panel[sizeKey], min, max);
}

function assertRightPanelSections() {
  if (!rightPanel || typeof rightPanel !== "object") {
    throw new Error("right_panel must be an object");
  }
  if (!rightPanel.sections || typeof rightPanel.sections !== "object") {
    throw new Error("right_panel.sections must be an object");
  }

  for (const section of ["facts", "selection", "code_refs", "notes", "receipts", "actions"]) {
    if (typeof rightPanel.sections[section] !== "boolean") {
      throw new Error(`right_panel.sections.${section} must be boolean`);
    }
  }
}

assertWindow();

const leftPolicy = assertPolicy("left", "width", "auto_hide_below_width", 120, 1200, 2400);
const rightPolicy = assertPolicy("right", "width", "auto_hide_below_width", 120, 1200, 2400);
const bottomPolicy = assertPolicy("bottom", "height", "auto_hide_below_height", 60, 1000, 1800);

assertPanel("left", "width", leftPolicy.min_width, leftPolicy.max_width);
assertPanel("right", "width", rightPolicy.min_width, rightPolicy.max_width);
assertPanel("bottom", "height", bottomPolicy.min_height, bottomPolicy.max_height);
assertRightPanelSections();

console.log(`${filePath}: ok`);
