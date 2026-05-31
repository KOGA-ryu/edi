#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const files = process.argv.slice(2);
if (files.length === 0) {
  files.push("data/review_subjects/draftsman_ui_taxonomy.json");
}

let failures = 0;

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

for (const file of files) {
  const errors = [];
  const fullPath = path.resolve(file);
  let document;

  try {
    document = JSON.parse(fs.readFileSync(fullPath, "utf8"));
  } catch (error) {
    console.error(`${file}: invalid JSON: ${error.message}`);
    failures += 1;
    continue;
  }

  if (!document.subject || typeof document.subject !== "object") {
    errors.push("document: missing subject object");
  } else {
    requireString(errors, document.subject, "subject_id", "subject");
    requireString(errors, document.subject, "label", "subject");
    requireString(errors, document.subject, "root_route_id", "subject");
  }

  requireArray(errors, document, "routes", "document");

  const routes = Array.isArray(document.routes) ? document.routes : [];
  const ids = new Set();

  for (const route of routes) {
    const context = `route ${route && route.route_id ? route.route_id : "<unknown>"}`;
    requireString(errors, route, "route_id", context);
    requireString(errors, route, "label", context);
    requireString(errors, route, "status", context);
    requireArray(errors, route, "children", context);
    requireArray(errors, route, "objects", context);
    requireArray(errors, route, "code_refs", context);
    requireArray(errors, route, "prompts", context);

    if (route.route_id) {
      if (ids.has(route.route_id)) {
        errors.push(`${context}: duplicate route_id`);
      }
      ids.add(route.route_id);
    }
  }

  if (document.subject && document.subject.root_route_id && !ids.has(document.subject.root_route_id)) {
    errors.push(`subject: root_route_id ${document.subject.root_route_id} does not exist`);
  }

  for (const route of routes) {
    if (route.parent && !ids.has(route.parent)) {
      errors.push(`route ${route.route_id}: parent ${route.parent} does not exist`);
    }
    for (const child of route.children || []) {
      if (!ids.has(child)) {
        errors.push(`route ${route.route_id}: child ${child} does not exist`);
      }
    }
  }

  if (errors.length > 0) {
    console.error(`${file}: failed`);
    for (const error of errors) {
      console.error(`  - ${error}`);
    }
    failures += 1;
  } else {
    console.log(`${file}: ok (${routes.length} routes)`);
  }
}

process.exit(failures === 0 ? 0 : 1);
