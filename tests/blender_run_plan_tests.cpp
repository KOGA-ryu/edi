#include "scripting/BlenderRunPlan.h"

#include <cassert>
#include <string>
#include <vector>

using namespace edi::scripting;

int main()
{
    // A complete plan: the argv is exactly --background --python <script> --
    // <output>, and the three paths are echoed for the caller.
    {
        const BlenderRunPlan plan = planBlenderRender(
            "/Applications/Blender.app/Contents/MacOS/Blender",
            "/tmp/scene.py",
            "/tmp/out.png");
        assert(plan.ok);
        assert(plan.executablePath == "/Applications/Blender.app/Contents/MacOS/Blender");
        assert(plan.outputImagePath == "/tmp/out.png");
        const std::vector<std::string> expected{
            "--background", "--python", "/tmp/scene.py", "--", "/tmp/out.png"};
        assert(plan.args == expected);
    }

    // No Blender configured: refuse with the setting to fix (not a crash, not a
    // silent no-op) — the executable-empty case is the "not set up yet" path.
    {
        const BlenderRunPlan plan = planBlenderRender("", "/tmp/scene.py", "/tmp/out.png");
        assert(!plan.ok);
        assert(plan.args.empty());
        assert(plan.message.find("blender.executable_path") != std::string::npos);
    }

    // No script / no output: each refuses with its own reason.
    {
        assert(!planBlenderRender("/blender", "", "/tmp/out.png").ok);
        assert(!planBlenderRender("/blender", "/tmp/scene.py", "").ok);
    }

    // The '--' separator is present and SINGLE — Blender stops parsing its flags
    // there, so the output path reaches the script, not Blender.
    {
        const BlenderRunPlan plan = planBlenderRender("/blender", "/s.py", "/o.png");
        int dashes = 0;
        std::size_t dashIndex = 0;
        for (std::size_t i = 0; i < plan.args.size(); ++i) {
            if (plan.args[i] == "--") {
                ++dashes;
                dashIndex = i;
            }
        }
        assert(dashes == 1);
        assert(dashIndex + 1 < plan.args.size() && plan.args[dashIndex + 1] == "/o.png");
    }

    return 0;
}
