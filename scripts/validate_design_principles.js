#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const files = process.argv.slice(2);
if (files.length === 0) {
  files.push("data/design_principles.json");
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
  if (!Array.isArray(object[key]) || object[key].length === 0) {
    errors.push(`${context}: missing non-empty array ${key}`);
  }
}

function requireObject(errors, object, key, context) {
  if (!isObject(object[key])) {
    errors.push(`${context}: missing object ${key}`);
  }
}

function uniqueIds(errors, items, key, context) {
  const ids = new Set();
  for (const item of Array.isArray(items) ? items : []) {
    if (!isObject(item)) {
      errors.push(`${context}: entries must be objects`);
      continue;
    }
    if (typeof item[key] !== "string" || item[key].trim().length === 0) {
      errors.push(`${context}: entry missing ${key}`);
      continue;
    }
    if (ids.has(item[key])) {
      errors.push(`${context}: duplicate ${key} ${item[key]}`);
    }
    ids.add(item[key]);
  }
  return ids;
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
  requireString(errors, document, "principles_id", "document");
  requireString(errors, document, "source_document", "document");
  requireArray(errors, document, "core_philosophy", "document");
  requireArray(errors, document, "dex_roles", "document");
  requireArray(errors, document, "design_profiles", "document");
  requireArray(errors, document, "review_checks", "document");

  if (document.source_document && !fs.existsSync(path.resolve(repoRoot, document.source_document))) {
    errors.push(`document: source_document not found: ${document.source_document}`);
  }

  const principleIds = uniqueIds(errors, document.core_philosophy, "id", "core_philosophy");
  for (const principle of Array.isArray(document.core_philosophy) ? document.core_philosophy : []) {
    if (!isObject(principle)) {
      continue;
    }
    requireString(errors, principle, "rule", `principle ${principle.id || "<unknown>"}`);
    requireArray(errors, principle, "avoid", `principle ${principle.id || "<unknown>"}`);
  }

  uniqueIds(errors, document.dex_roles, "role_id", "dex_roles");
  for (const role of Array.isArray(document.dex_roles) ? document.dex_roles : []) {
    if (!isObject(role)) {
      continue;
    }
    const context = `role ${role.role_id || "<unknown>"}`;
    requireString(errors, role, "purpose", context);
    requireArray(errors, role, "must_read", context);
    requireArray(errors, role, "must_produce", context);
    requireArray(errors, role, "must_not_do", context);
  }

  uniqueIds(errors, document.design_profiles, "profile_id", "design_profiles");
  for (const profile of Array.isArray(document.design_profiles) ? document.design_profiles : []) {
    if (!isObject(profile)) {
      continue;
    }
    const context = `design profile ${profile.profile_id || "<unknown>"}`;
    requireString(errors, profile, "purpose", context);
    requireArray(errors, profile, "inherits", context);
    requireObject(errors, profile, "defaults", context);
    for (const inherited of Array.isArray(profile.inherits) ? profile.inherits : []) {
      if (!principleIds.has(inherited)) {
        errors.push(`${context}: unknown inherited principle ${inherited}`);
      }
    }
  }

  uniqueIds(errors, document.review_checks, "check_id", "review_checks");
  for (const check of Array.isArray(document.review_checks) ? document.review_checks : []) {
    if (!isObject(check)) {
      continue;
    }
    const context = `review check ${check.check_id || "<unknown>"}`;
    requireString(errors, check, "question", context);
    requireString(errors, check, "fail_if", context);
  }

  if (errors.length > 0) {
    console.error(`${file}: failed`);
    for (const error of errors) {
      console.error(`  - ${error}`);
    }
    failures += 1;
  } else {
    console.log(`${file}: ok (${document.core_philosophy.length} principles, ${document.design_profiles.length} profiles)`);
  }
}

process.exit(failures === 0 ? 0 : 1);
