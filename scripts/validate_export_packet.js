#!/usr/bin/env node

const crypto = require("crypto");
const fs = require("fs");
const path = require("path");

const packetDirs = process.argv.slice(2);
if (packetDirs.length === 0) {
  console.error("usage: scripts/validate_export_packet.js <packet-dir> [...]");
  process.exit(1);
}

let failures = 0;

function isObject(value) {
  return value && typeof value === "object" && !Array.isArray(value);
}

function sha256(filePath) {
  return crypto.createHash("sha256").update(fs.readFileSync(filePath)).digest("hex");
}

function packetPath(packetDir, relativePath) {
  const clean = path.normalize(String(relativePath || ""));
  if (!clean || path.isAbsolute(clean) || clean === ".." || clean.startsWith(`..${path.sep}`)) {
    return null;
  }
  return path.resolve(packetDir, clean);
}

function requireFile(errors, packetDir, relativePath, context) {
  const fullPath = packetPath(packetDir, relativePath);
  if (!fullPath) {
    errors.push(`${context}: invalid relative path ${relativePath}`);
    return null;
  }
  if (!fs.existsSync(fullPath) || !fs.statSync(fullPath).isFile()) {
    errors.push(`${context}: file not found ${relativePath}`);
    return null;
  }
  return fullPath;
}

function validateHash(errors, packetDir, relativePath, expectedHash, context) {
  const fullPath = requireFile(errors, packetDir, relativePath, context);
  if (!fullPath) {
    return;
  }
  if (typeof expectedHash !== "string" || !/^[a-f0-9]{64}$/.test(expectedHash)) {
    errors.push(`${context}: sha256 must be a lowercase 64-character hex string`);
    return;
  }
  const actualHash = sha256(fullPath);
  if (actualHash !== expectedHash) {
    errors.push(`${context}: sha256 mismatch for ${relativePath}`);
  }
}

function parseIndex(errors, packetDir) {
  const indexPath = requireFile(errors, packetDir, "index.txt", "index");
  if (!indexPath) {
    return;
  }
  const lines = fs.readFileSync(indexPath, "utf8").split(/\r?\n/);
  let hashLines = 0;
  for (const line of lines) {
    const match = line.match(/^([a-f0-9]{64})\s+(.+)$/);
    if (!match) {
      continue;
    }
    hashLines += 1;
    validateHash(errors, packetDir, match[2], match[1], `index ${match[2]}`);
  }
  if (hashLines === 0) {
    errors.push("index: no sha256 file records found");
  }
}

for (const packetDirInput of packetDirs) {
  const errors = [];
  const packetDir = path.resolve(packetDirInput);

  if (!fs.existsSync(packetDir) || !fs.statSync(packetDir).isDirectory()) {
    console.error(`${packetDirInput}: failed`);
    console.error("  - packet: directory not found");
    failures += 1;
    continue;
  }

  const manifestPath = path.join(packetDir, "manifest.json");
  let manifest;
  try {
    manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
  } catch (error) {
    console.error(`${packetDirInput}: failed`);
    console.error(`  - manifest: invalid or missing manifest.json: ${error.message}`);
    failures += 1;
    continue;
  }

  if (manifest.schema_version !== 1) {
    errors.push("manifest: schema_version must be 1");
  }
  if (typeof manifest.packet_type !== "string" || manifest.packet_type.trim().length === 0) {
    errors.push("manifest: missing packet_type");
  }
  if (!Array.isArray(manifest.documents)) {
    errors.push("manifest: documents must be an array");
  }
  if (!Array.isArray(manifest.files)) {
    errors.push("manifest: files must be an array");
  }

  const seenDocumentIds = new Set();
  for (const document of Array.isArray(manifest.documents) ? manifest.documents : []) {
    const context = `document ${document && document.id ? document.id : "<unknown>"}`;
    if (!isObject(document)) {
      errors.push(`${context}: must be object`);
      continue;
    }
    if (typeof document.id !== "string" || document.id.trim().length === 0) {
      errors.push(`${context}: missing id`);
    } else if (seenDocumentIds.has(document.id)) {
      errors.push(`${context}: duplicate id`);
    } else {
      seenDocumentIds.add(document.id);
    }
    if (typeof document.exported_path !== "string" || document.exported_path.trim().length === 0) {
      errors.push(`${context}: missing exported_path`);
    } else {
      validateHash(errors, packetDir, document.exported_path, document.sha256, context);
    }
  }

  const seenFilePaths = new Set();
  for (const fileRecord of Array.isArray(manifest.files) ? manifest.files : []) {
    const context = `file ${fileRecord && fileRecord.path ? fileRecord.path : "<unknown>"}`;
    if (!isObject(fileRecord)) {
      errors.push(`${context}: must be object`);
      continue;
    }
    if (typeof fileRecord.path !== "string" || fileRecord.path.trim().length === 0) {
      errors.push(`${context}: missing path`);
      continue;
    }
    if (seenFilePaths.has(fileRecord.path)) {
      errors.push(`${context}: duplicate path`);
    }
    seenFilePaths.add(fileRecord.path);
    validateHash(errors, packetDir, fileRecord.path, fileRecord.sha256, context);
  }

  parseIndex(errors, packetDir);

  if (manifest.packet_type === "dex_handoff") {
    for (const required of ["AGENT_README.txt", "prompt.txt", "context.txt", "all_documents.txt"]) {
      requireFile(errors, packetDir, required, "dex_handoff");
    }
    if (!isObject(manifest.handoff_files)) {
      errors.push("dex_handoff: missing handoff_files object");
    }
  }

  if (errors.length > 0) {
    console.error(`${packetDirInput}: failed`);
    for (const error of errors) {
      console.error(`  - ${error}`);
    }
    failures += 1;
  } else {
    console.log(`${packetDirInput}: ok (${manifest.packet_type}, ${seenDocumentIds.size} documents)`);
  }
}

process.exit(failures > 0 ? 1 : 0);
