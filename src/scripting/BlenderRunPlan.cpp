#include "scripting/BlenderRunPlan.h"

namespace edi::scripting {

BlenderRunPlan planBlenderRender(const std::string &executablePath,
                                 const std::string &scriptPath,
                                 const std::string &outputImagePath)
{
    BlenderRunPlan plan;
    plan.executablePath = executablePath;
    plan.outputImagePath = outputImagePath;

    // The executable is the configured Blender path; empty means the user has
    // not pointed edi at a Blender yet — refuse with the setting to fix.
    if (executablePath.empty()) {
        plan.message = "Blender executable not configured — set blender.executable_path in settings";
        return plan;
    }
    if (scriptPath.empty()) {
        plan.message = "no script to build";
        return plan;
    }
    if (outputImagePath.empty()) {
        plan.message = "no output image path";
        return plan;
    }

    // --background: no GUI. --python <script>: run the user's bpy. The lone '--'
    // hands everything after it to the script (Blender stops parsing its own
    // flags there), so the script reads sys.argv after '--' to learn where to
    // render. ONE post-'--' arg by design: the output PNG path.
    plan.args = {"--background", "--python", scriptPath, "--", outputImagePath};
    plan.ok = true;
    plan.message = "ready";
    return plan;
}

} // namespace edi::scripting
