#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const files = process.argv.slice(2);
if (files.length === 0) {
  files.push("data/project_profiles/draftsman_blank.json");
}

let failures = 0;

function requireObject(errors, object, key, context) {
  if (!object[key] || typeof object[key] !== "object" || Array.isArray(object[key])) {
    errors.push(`${context}: missing object ${key}`);
  }
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

function requireBoolean(errors, object, key, context) {
  if (typeof object[key] !== "boolean") {
    errors.push(`${context}: missing boolean ${key}`);
  }
}

function optionalString(errors, object, key, context) {
  if (object[key] !== undefined && typeof object[key] !== "string") {
    errors.push(`${context}: ${key} must be a string when present`);
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

  if (document.schema_version !== 1) {
    errors.push("document: schema_version must be 1");
  }

  requireObject(errors, document, "profile", "document");
  requireArray(errors, document, "activity_modes", "document");
  requireObject(errors, document, "left_panel", "document");
  requireObject(errors, document, "main_workspace", "document");
  requireObject(errors, document, "right_inspector", "document");
  requireObject(errors, document, "bottom_panel", "document");
  requireObject(errors, document, "data_sources", "document");
  requireObject(errors, document, "write_policy", "document");

  if (document.profile) {
    requireString(errors, document.profile, "profile_id", "profile");
    requireString(errors, document.profile, "label", "profile");
    requireString(errors, document.profile, "type", "profile");
    requireString(errors, document.profile, "default_activity", "profile");
  }

  const modeIds = new Set();
  for (const mode of Array.isArray(document.activity_modes) ? document.activity_modes : []) {
    const context = `activity mode ${mode && mode.id ? mode.id : "<unknown>"}`;
    requireString(errors, mode, "id", context);
    requireString(errors, mode, "label", context);
    requireString(errors, mode, "icon", context);
    requireString(errors, mode, "tooltip", context);
    requireBoolean(errors, mode, "enabled", context);
    if (mode.id) {
      if (modeIds.has(mode.id)) {
        errors.push(`${context}: duplicate id`);
      }
      modeIds.add(mode.id);
    }
  }

  if (document.profile && document.profile.default_activity && !modeIds.has(document.profile.default_activity)) {
    errors.push(`profile: default_activity ${document.profile.default_activity} is not declared`);
  }

  if (document.left_panel) {
    requireArray(errors, document.left_panel, "project_rows", "left_panel");
    for (const row of Array.isArray(document.left_panel.project_rows) ? document.left_panel.project_rows : []) {
      requireString(errors, row, "label", "left_panel.project_rows row");
      requireString(errors, row, "meta", "left_panel.project_rows row");
    }
  }

  if (document.main_workspace) {
    requireString(errors, document.main_workspace, "feature", "main_workspace");
  }

  if (document.right_inspector) {
    requireString(errors, document.right_inspector, "source", "right_inspector");
    requireObject(errors, document.right_inspector, "sections", "right_inspector");
    for (const section of ["facts", "selection", "code_refs", "notes", "receipts", "actions"]) {
      if (document.right_inspector.sections && typeof document.right_inspector.sections[section] !== "boolean") {
        errors.push(`right_inspector.sections.${section} must be boolean`);
      }
    }
  }

  if (document.bottom_panel) {
    requireArray(errors, document.bottom_panel, "tabs", "bottom_panel");
    for (const tab of Array.isArray(document.bottom_panel.tabs) ? document.bottom_panel.tabs : []) {
      if (typeof tab !== "string" || tab.trim().length === 0) {
        errors.push("bottom_panel.tabs entries must be non-empty strings");
      }
    }
  }

  if (document.data_sources) {
    const usesBlankCanvas = document.main_workspace && document.main_workspace.feature === "blank_canvas";
    if (usesBlankCanvas) {
      optionalString(errors, document.data_sources, "review_subject", "data_sources");
    } else {
      requireString(errors, document.data_sources, "review_subject", "data_sources");
    }
    optionalString(errors, document.data_sources, "review_notes", "data_sources");
    if (!usesBlankCanvas && typeof document.data_sources.review_notes === "string" && document.data_sources.review_notes.trim().length === 0) {
      errors.push("data_sources: review_notes is required unless main_workspace.feature is blank_canvas");
    }
    if (!usesBlankCanvas && typeof document.data_sources.review_subject === "string" && document.data_sources.review_subject.trim().length === 0) {
      errors.push("data_sources: review_subject is required unless main_workspace.feature is blank_canvas");
    }
  }

  if (document.write_policy) {
    requireBoolean(errors, document.write_policy, "writes_enabled", "write_policy");
    requireString(errors, document.write_policy, "note_storage", "write_policy");
  }

  if (errors.length > 0) {
    console.error(`${file}: failed`);
    for (const error of errors) {
      console.error(`  - ${error}`);
    }
    failures += 1;
  } else {
    console.log(`${file}: ok (${document.profile.profile_id})`);
  }
}

process.exit(failures === 0 ? 0 : 1);
