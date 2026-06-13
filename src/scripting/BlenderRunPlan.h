#pragma once

#include <string>
#include <vector>

namespace edi::scripting {

// The pure, effect-free half of "build a bpy script in Blender": given the three
// paths involved, decide the exact subprocess to run, or refuse with a named
// reason. No filesystem access, no process launch — the controller's
// ProcessRunStore performs the effect from this plan, so the decision stays
// unit-testable without a Blender install. (Sibling of ScriptCommandBridge's
// planScriptCommands: plan first, refuse legibly, effect later.)
struct BlenderRunPlan {
    bool ok = false;
    std::string message;             // a named reason when !ok; a summary when ok
    std::string executablePath;      // the Blender binary to spawn
    std::vector<std::string> args;   // argv after the executable
    std::string outputImagePath;     // where the script is told to render the PNG
};

// Plan a headless render of a raw bpy script: the user's script runs inside
// Blender and renders to outputImagePath. The argv is
//   --background --python <script> -- <outputImagePath>
// so the script receives the output path as its sole post-'--' argument (a
// starter template shows the two render lines that consume it). Refuses (ok =
// false, named message) when the executable, script, or output path is empty —
// the executable being empty is the "Blender not configured yet" case.
BlenderRunPlan planBlenderRender(const std::string &executablePath,
                                 const std::string &scriptPath,
                                 const std::string &outputImagePath);

} // namespace edi::scripting
