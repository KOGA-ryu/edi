#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const profilePath = process.argv[2] || "data/project_profiles/draftsman_game_guy_map_editor.json";
const repoRoot = process.cwd();
let failures = 0;

function fail(message) {
  console.error(message);
  failures += 1;
}

function readJson(file) {
  return JSON.parse(fs.readFileSync(path.resolve(repoRoot, file), "utf8"));
}

function resolveDataPath(relativePath) {
  return path.resolve(repoRoot, relativePath);
}

let profile;
try {
  profile = readJson(profilePath);
} catch (error) {
  fail(`${profilePath}: invalid profile JSON: ${error.message}`);
}

if (profile) {
  const sources = profile.data_sources || {};
  const csvPath = sources.map_csv || "";
  const metadataPath = sources.cell_metadata || "";

  if (!csvPath) {
    fail(`${profilePath}: missing data_sources.map_csv`);
  }
  if (!metadataPath) {
    fail(`${profilePath}: missing data_sources.cell_metadata`);
  }

  if (csvPath) {
    const fullCsvPath = resolveDataPath(csvPath);
    if (!fs.existsSync(fullCsvPath)) {
      fail(`${csvPath}: file not found`);
    } else {
      const rows = fs.readFileSync(fullCsvPath, "utf8")
        .split(/\r?\n/)
        .map((line) => line.trim())
        .filter(Boolean)
        .map((line) => line.split(",").map((cell) => cell.trim()));
      if (rows.length === 0) {
        fail(`${csvPath}: grid is empty`);
      }
      const width = rows[0] ? rows[0].length : 0;
      if (width === 0) {
        fail(`${csvPath}: first row is empty`);
      }
      rows.forEach((row, index) => {
        if (row.length !== width) {
          fail(`${csvPath}: row ${index} has ${row.length} cells, expected ${width}`);
        }
        row.forEach((token, col) => {
          if (!/^[a-z_]+$/.test(token)) {
            fail(`${csvPath}: invalid token at ${index},${col}: ${token}`);
          }
        });
      });
    }
  }

  if (metadataPath) {
    const fullMetadataPath = resolveDataPath(metadataPath);
    if (!fs.existsSync(fullMetadataPath)) {
      fail(`${metadataPath}: file not found`);
    } else {
      const lines = fs.readFileSync(fullMetadataPath, "utf8")
        .split(/\r?\n/)
        .map((line) => line.trim())
        .filter(Boolean);
      lines.forEach((line, index) => {
        let item;
        try {
          item = JSON.parse(line);
        } catch (error) {
          fail(`${metadataPath}: line ${index + 1} invalid JSON: ${error.message}`);
          return;
        }
        if (!Number.isInteger(item.row) || !Number.isInteger(item.col)) {
          fail(`${metadataPath}: line ${index + 1} row/col must be integers`);
        }
        if (typeof item.intent !== "string" || item.intent.trim().length === 0) {
          fail(`${metadataPath}: line ${index + 1} missing intent`);
        }
        if (!Array.isArray(item.tags)) {
          fail(`${metadataPath}: line ${index + 1} tags must be array`);
        }
        if (item.code_refs !== undefined && !Array.isArray(item.code_refs)) {
          fail(`${metadataPath}: line ${index + 1} code_refs must be array when present`);
        }
      });
    }
  }
}

if (failures === 0) {
  console.log(`${profilePath}: ok (csv_map_editor)`);
}

process.exit(failures === 0 ? 0 : 1);
