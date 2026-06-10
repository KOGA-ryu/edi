#include "widgets/ShellHost.h"

#include <QApplication>
#include <QWidget>

#include <cassert>

using namespace edi::shell;

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // A fake feature proves the host is generic: nothing here knows about
    // drafting. The factory labels each widget with the slot it was asked to
    // fill, so the test can verify per-slot dispatch.
    FeatureContext context;
    FeatureContext *seenContext = nullptr;
    FeatureDescriptor fake;
    fake.id = QStringLiteral("fake");
    fake.label = QStringLiteral("Fake feature");
    fake.supportedSlots = {ShellSlot::Left, ShellSlot::Main};
    fake.buildPanel = [&seenContext](ShellSlot slot, FeatureContext &ctx) -> QWidget * {
        seenContext = &ctx;
        auto *widget = new QWidget;
        widget->setObjectName(slot == ShellSlot::Left
            ? QStringLiteral("fakeLeft")
            : QStringLiteral("fakeMain"));
        return widget;
    };

    FeatureDescriptor broken;
    broken.id = QStringLiteral("broken");
    broken.label = QStringLiteral("Null factory");
    broken.supportedSlots = {ShellSlot::Bottom};
    // buildPanel left null on purpose.

    FeatureRegistry registry;
    registry.features = {fake, broken};

    // Registry lookup is by id.
    assert(findFeature(registry, QStringLiteral("fake")) != nullptr);
    assert(findFeature(registry, QStringLiteral("fake"))->label == QStringLiteral("Fake feature"));
    assert(findFeature(registry, QStringLiteral("missing")) == nullptr);

    // A layout exercising every skip rule alongside the two good bindings:
    // unknown feature, slot the feature cannot fill, feature with no factory.
    WorkspaceLayout layout;
    layout.id = QStringLiteral("test_job");
    layout.bindings = {
        {ShellSlot::Left, QStringLiteral("fake")},
        {ShellSlot::Main, QStringLiteral("fake")},
        {ShellSlot::Right, QStringLiteral("missing")},
        {ShellSlot::Right, QStringLiteral("fake")},     // fake cannot fill Right
        {ShellSlot::Bottom, QStringLiteral("broken")},  // null factory
    };

    const std::vector<MountedSlot> mounted = mountWorkspaceLayout(layout, registry, context);

    // Only the two valid bindings produce widgets; bad bindings degrade to an
    // emptier shell instead of crashing.
    assert(mounted.size() == 2);
    assert(seenContext == &context); // the shared bus is passed through, not copied

    QWidget *left = mountedSlotWidget(mounted, ShellSlot::Left);
    QWidget *main = mountedSlotWidget(mounted, ShellSlot::Main);
    assert(left != nullptr && left->objectName() == QStringLiteral("fakeLeft"));
    assert(main != nullptr && main->objectName() == QStringLiteral("fakeMain"));
    assert(mountedSlotWidget(mounted, ShellSlot::Right) == nullptr);
    assert(mountedSlotWidget(mounted, ShellSlot::Bottom) == nullptr);

    delete left;
    delete main;
    return 0;
}
