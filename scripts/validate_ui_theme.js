#!/usr/bin/env node

const fs = require("fs");

const file = process.argv[2] || "data/ui_theme.json";
const hex = /^#[0-9a-fA-F]{6}$/;
const supportedFields = new Set([
  "theme_mode",
  "base",
  "surface",
  "accent",
  "text",
  "ui_font",
  "code_font",
  "ui_font_size",
  "code_font_size",
]);
const errors = [];

let theme;
try {
  theme = JSON.parse(fs.readFileSync(file, "utf8"));
} catch (error) {
  console.error(`${file}: invalid JSON: ${error.message}`);
  process.exit(1);
}

if (!theme || typeof theme !== "object") {
  errors.push("theme must be an object");
}

if (!["light", "dark", "system"].includes(theme.theme_mode)) {
  errors.push("theme_mode must be light, dark, or system");
}

for (const key of ["base", "surface", "accent", "text"]) {
  if (typeof theme[key] !== "string" || !hex.test(theme[key])) {
    errors.push(`${key} must be #RRGGBB`);
  }
}

for (const key of ["ui_font", "code_font"]) {
  if (typeof theme[key] !== "string") {
    errors.push(`${key} must be a string`);
  }
}

for (const key of ["ui_font_size", "code_font_size"]) {
  const value = theme[key];
  if (!Number.isInteger(value) || value < 9 || value > 28) {
    errors.push(`${key} must be an integer from 9 to 28`);
  }
}

for (const key of Object.keys(theme)) {
  if (!supportedFields.has(key)) {
    errors.push(`${key} is not a supported theme field`);
  }
}

if (errors.length > 0) {
  console.error(`${file}: failed`);
  for (const error of errors) {
    console.error(`  - ${error}`);
  }
  process.exit(1);
}

console.log(`${file}: ok`);
