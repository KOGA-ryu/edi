#!/usr/bin/env node

const fs = require("fs");

const file = process.argv[2] || "data/ui_theme.json";
const hex = /^#[0-9a-fA-F]{6}$/;
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

for (const key of ["theme_id", "label"]) {
  if (typeof theme[key] !== "string" || theme[key].trim().length === 0) {
    errors.push(`missing string ${key}`);
  }
}

if (!theme.colors || typeof theme.colors !== "object") {
  errors.push("missing colors object");
} else {
  for (const key of ["base", "surface", "accent", "text"]) {
    if (typeof theme.colors[key] !== "string" || !hex.test(theme.colors[key])) {
      errors.push(`colors.${key} must be #RRGGBB`);
    }
  }
}

if (!theme.typography || typeof theme.typography !== "object") {
  errors.push("missing typography object");
} else {
  for (const key of ["ui_font_size", "code_font_size"]) {
    const value = theme.typography[key];
    if (!Number.isInteger(value) || value < 10 || value > 24) {
      errors.push(`typography.${key} must be an integer from 10 to 24`);
    }
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
