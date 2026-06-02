#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const files = process.argv.slice(2);
if (files.length === 0) {
  files.push("data/shell_surface_map.json");
}

let failures = 0;

function isObject(value) {
  return value && typeof value === "object" && !Array.isArray(value);
}

function requireString(errors, object, key, context) {
  if (typeof object[key] !== "string" || object[key].trim().length === 0) {
    errors.push(`${context}: missing string ${key}`);
  }
}

function requireArray(errors, object, key, context) {
  if (!Array.isArray(object[key])) {
    errors.push(`${context}: missing array ${key}`);
  }
}

function fileExists(repoRoot, relativePath) {
  return fs.existsSync(path.resolve(repoRoot, relativePath));
}

for (const file of files) {
  const errors = [];
  const fullPath = path.resolve(file);
  const repoRoot = path.dirname(path.dirname(fullPath));
  let document;

  try {
    document = JSON.parse(fs.readFileSync(fullPath, "utf8"));
  } catch (error) {
    console.error(`${file}: invalid JSON: ${error.message}`);
    failures += 1;
    continue;
  }

  if (document.schema_version !== 1) {
    errors.push("document: schema_version must be 1");
  }
  requireString(errors, document, "map_id", "document");
  requireString(errors, document, "default_profile", "document");
  requireArray(errors, document, "entry_documents", "document");
  requireArray(errors, document, "profiles", "document");
  requireArray(errors, document, "surfaces", "document");
  requireArray(errors, document, "integration_workflow", "document");

  if (document.default_profile && !fileExists(repoRoot, document.default_profile)) {
    errors.push(`document: default_profile not found: ${document.default_profile}`);
  }

  for (const entry of Array.isArray(document.entry_documents) ? document.entry_documents : []) {
    if (typeof entry !== "string" || entry.trim().length === 0) {
      errors.push("entry_documents: entries must be non-empty strings");
    } else if (!fileExists(repoRoot, entry)) {
      errors.push(`entry_documents: file not found: ${entry}`);
    }
  }

  const profileIds = new Set();
  for (const profile of Array.isArray(document.profiles) ? document.profiles : []) {
    const context = `profile ${profile && profile.profile_id ? profile.profile_id : "<unknown>"}`;
    if (!isObject(profile)) {
      errors.push(`${context}: must be object`);
      continue;
    }
    requireString(errors, profile, "profile_id", context);
    requireString(errors, profile, "path", context);
    requireString(errors, profile, "purpose", context);
    requireString(errors, profile, "default_activity", context);
    requireArray(errors, profile, "enabled_features", context);
    if (profile.profile_id) {
      if (profileIds.has(profile.profile_id)) {
        errors.push(`${context}: duplicate profile_id`);
      }
      profileIds.add(profile.profile_id);
    }
    if (profile.path && !fileExists(repoRoot, profile.path)) {
      errors.push(`${context}: path not found: ${profile.path}`);
    }
  }

  const surfaceIds = new Set();
  for (const surface of Array.isArray(document.surfaces) ? document.surfaces : []) {
    const context = `surface ${surface && surface.surface_id ? surface.surface_id : "<unknown>"}`;
    if (!isObject(surface)) {
      errors.push(`${context}: must be object`);
      continue;
    }
    requireString(errors, surface, "surface_id", context);
    requireString(errors, surface, "kind", context);
    requireString(errors, surface, "owner_file", context);
    requireArray(errors, surface, "activity_modes", context);
    requireArray(errors, surface, "inputs", context);
    requireArray(errors, surface, "may_edit", context);
    requireArray(errors, surface, "must_not_edit", context);
    requireArray(errors, surface, "proof", context);
    if (surface.surface_id) {
      if (surfaceIds.has(surface.surface_id)) {
        errors.push(`${context}: duplicate surface_id`);
      }
      surfaceIds.add(surface.surface_id);
    }
    if (surface.owner_file && !fileExists(repoRoot, surface.owner_file)) {
      errors.push(`${context}: owner_file not found: ${surface.owner_file}`);
    }
    for (const proofPath of Array.isArray(surface.proof) ? surface.proof : []) {
      if (typeof proofPath !== "string" || proofPath.trim().length === 0) {
        errors.push(`${context}: proof entries must be non-empty strings`);
      } else if (!fileExists(repoRoot, proofPath)) {
        errors.push(`${context}: proof not found: ${proofPath}`);
      }
    }
  }

  if (!surfaceIds.has("blank_canvas")) {
    errors.push("surfaces: missing blank_canvas");
  }
  if (!surfaceIds.has("main_workspace")) {
    errors.push("surfaces: missing main_workspace");
  }

  if (errors.length > 0) {
    console.error(`${file}: failed`);
    for (const error of errors) {
      console.error(`  - ${error}`);
    }
    failures += 1;
  } else {
    console.log(`${file}: ok (${document.surfaces.length} surfaces)`);
  }
}

process.exit(failures === 0 ? 0 : 1);
