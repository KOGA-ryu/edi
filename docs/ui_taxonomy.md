# Draftsman UI Taxonomy

The UI taxonomy turns the app into its own review subject:

```text
Draftsman UI
  Top Chrome
  Activity Rail
  Left Panel
  Main Workspace
  Right Panel
  Bottom Panel
  Status Bar
  Settings
  Feature Surfaces
  Data Contracts
  Proof / Review
```

Each route has a purpose, status, review prompts, code references, child routes, and human notes. The current implementation loads route taxonomy from `data/review_subjects/draftsman_ui_taxonomy.json`; human notes and status overrides remain in-memory until the write contract is approved.
