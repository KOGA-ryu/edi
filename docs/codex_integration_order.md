# Codex Integration Order

Integrate this split QML shell into `/Users/kogaryu/ui`.

Goal: replace monolithic generated QML as the human edit surface with a component-per-region QML source structure.

Hard rules:

- `Main.qml` composes regions only.
- Each major region gets its own file.
- Shared style constants come from `style/UiStyle.qml`.
- Reusable controls live in `components/`.
- Generated output is not the human edit surface.
- If the renderer is generated, update the generator to preserve this split structure.

Target structure:

```text
renderers/qt_qml_static/src/
  Main.qml
  style/
    qmldir
    UiStyle.qml
  regions/
    ActivityRail.qml
    LeftPanel.qml
    MainWorkspace.qml
    RightPanel.qml
    BottomPanel.qml
    StatusBar.qml
  components/
    UiButton.qml
    UiListRow.qml
    UiPanel.qml
    UiSectionHeader.qml
    UiTab.qml
  runtime/
    RuntimeController.qml
```

Acceptance:

- Existing monolithic `Main.qml` is no longer the source humans edit.
- The shell still runs in Qt/QML.
- A human can change panel sizes in `UiStyle.qml`.
- A human can change left/right/main/bottom panel internals by opening one obvious region file.
- Repeated color, spacing, font, radius, and border values are not scattered across region files.
