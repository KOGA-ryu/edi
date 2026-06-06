#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const file = process.argv[2] || "data/text_editor/documents.json";
const fullPath = path.resolve(file);
const errors = [];

function requireString(object, key, context) {
  if (typeof object[key] !== "string" || object[key].trim().length === 0) {
    errors.push(`${context}: missing string ${key}`);
  }
}

let document;
try {
  document = JSON.parse(fs.readFileSync(fullPath, "utf8"));
} catch (error) {
  console.error(`${file}: invalid JSON: ${error.message}`);
  process.exit(1);
}

if (document.schema_version !== 1) {
  errors.push("document: schema_version must be 1");
}

if (!Array.isArray(document.documents)) {
  errors.push("document: documents must be an array");
}

const seenIds = new Set();
const manifestDir = path.dirname(fullPath);
for (const item of Array.isArray(document.documents) ? document.documents : []) {
  const context = `document ${item && item.id ? item.id : "<unknown>"}`;
  requireString(item, "id", context);
  requireString(item, "name", context);
  requireString(item, "language", context);
  requireString(item, "path", context);

  if (typeof item.id === "string") {
    if (seenIds.has(item.id)) {
      errors.push(`${context}: duplicate id`);
    }
    seenIds.add(item.id);
  }

  if (typeof item.path === "string") {
    const clean = path.normalize(item.path);
    if (path.isAbsolute(item.path) || clean === ".." || clean.startsWith(`..${path.sep}`)) {
      errors.push(`${context}: path must stay relative to the manifest`);
    } else if (!fs.existsSync(path.resolve(manifestDir, clean))) {
      errors.push(`${context}: text file does not exist`);
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

console.log(`${file}: ok (${seenIds.size} documents)`);
