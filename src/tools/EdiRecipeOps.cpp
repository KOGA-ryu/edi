// edi_recipe_ops — the op pipeline's command-line entry, mirroring
// EdiPlotJobReport's shape (widget-free, load → process → report, non-zero on
// any refusal). It is the CLI half of R1-B05's "make the op pipeline
// reachable": load an ops TOML, optionally resolve it against a drawing, then
// run it through the SAME gates the shell verbs use — nothing downstream of
// resolve ever sees an unresolved stream, and every refusal names its offender.
//
//   edi_recipe_ops <ops.toml> [--resolve <drawing.edidraw>]
//                  [--previews <dir>] [--compiled-out <path>]
#include "recipe/RecipeOps.h"
#include "recipe/RecipeOpsAscii.h"
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include "drafting/DraftingGrid.h"
#include "drafting/DraftingSerialize.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace edi::recipe;
using namespace edi::drafting;

bool readTextFile(const std::string &path, std::string &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

bool readByteFile(const std::string &path, std::vector<std::uint8_t> &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool writeTextFile(const std::string &path, const std::string &text)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(out);
}

void printUsage()
{
    std::cerr << "usage: edi_recipe_ops <ops.toml> [--resolve <drawing.edidraw>]"
                 " [--previews <dir>] [--compiled-out <path>]\n";
}

} // namespace

int main(int argc, char **argv)
{
    std::string opsPath;
    std::string resolvePath;
    std::string previewsDir;
    std::string compiledOut;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--resolve" && index + 1 < argc) {
            resolvePath = argv[++index];
            continue;
        }
        if (arg == "--previews" && index + 1 < argc) {
            previewsDir = argv[++index];
            continue;
        }
        if (arg == "--compiled-out" && index + 1 < argc) {
            compiledOut = argv[++index];
            continue;
        }
        if (!arg.empty() && arg.front() == '-') {
            printUsage();
            return 2;
        }
        if (opsPath.empty()) {
            opsPath = arg;
            continue;
        }
        printUsage();
        return 2;
    }
    if (opsPath.empty()) {
        printUsage();
        return 2;
    }

    // Load the stream through the strict reader — refusals name their offender.
    std::string opsText;
    if (!readTextFile(opsPath, opsText)) {
        std::cerr << "edi_recipe_ops: cannot read " << opsPath << "\n";
        return 1;
    }
    const OpStreamParseResult parsed = recipeOpsFromToml(opsText, opsPath);
    if (!parsed.ok) {
        std::cerr << "load refused: " << parsed.message << "\n";
        return 1;
    }
    RecipeOpStream stream = parsed.stream;

    // Optional resolve against a drawing. The document carries no grid of its
    // own (the shell verb resolves against the live controller's grid), so the
    // default grid stands here — a CLI limitation recorded in the B05 report.
    if (!resolvePath.empty()) {
        std::vector<std::uint8_t> bytes;
        if (!readByteFile(resolvePath, bytes)) {
            std::cerr << "edi_recipe_ops: cannot read " << resolvePath << "\n";
            return 1;
        }
        const edi::formats::FormatResult<DraftingDocument> decoded =
            decodeDraftingDocument(bytes, resolvePath);
        if (!decoded.ok || !decoded.value.has_value()) {
            std::cerr << "drawing refused: " << decoded.message << "\n";
            return 1;
        }
        const DraftingGridProjection grid = projectDraftingGrid(defaultDraftingGridSettings());
        const OpResolveResult resolved = resolveRecipeOps(stream, *decoded.value, grid);
        if (!resolved.ok) {
            std::cerr << "resolve refused:\n";
            for (const OpResolveFinding &finding : resolved.findings) {
                std::cerr << "  " << finding.key << ": " << finding.message << "\n";
            }
            return 1;
        }
        stream = resolved.stream;
    }

    // The gate (decision 1): nothing downstream may see an unresolved stream.
    if (!recipeOpsResolved(stream)) {
        std::cerr << "not resolved: the stream still carries measurement bindings or a"
                     " lathe reference; pass --resolve <drawing.edidraw>\n";
        return 1;
    }

    // Compile (every AddProfileMoulding expands to explicit points).
    const RecipeCompileResult compiled = compileRecipeOps(stream.ops);
    if (!compiled.ok) {
        std::cerr << "compile refused: " << compiled.message << "\n";
        return 1;
    }

    // Validate: an error refuses the whole run (warnings are reported, not
    // fatal — the same Severity split the validator already draws).
    const OpValidationReport report = validateRecipeOps(compiled.ops);
    if (!report.ok) {
        std::cerr << "validation refused:\n";
        for (const OpFinding &finding : report.findings) {
            if (finding.severity == OpFinding::Severity::Error) {
                std::cerr << "  error " << finding.code << ": " << finding.message << "\n";
            }
        }
        return 1;
    }

    // Render the three projections to <dir>/{front,side,top}.txt (the same
    // bytes the recipe_ops_ascii suite pins against the committed previews).
    if (!previewsDir.empty()) {
        const struct {
            AsciiProjection projection;
            const char *file;
        } views[] = {
            {AsciiProjection::Front, "front.txt"},
            {AsciiProjection::Side, "side.txt"},
            {AsciiProjection::Top, "top.txt"},
        };
        for (const auto &view : views) {
            const AsciiRenderResult render = renderOpsProjection(compiled.ops, view.projection);
            if (!render.ok) {
                std::cerr << "preview refused (" << view.file << "): " << render.message << "\n";
                return 1;
            }
            const std::string path = previewsDir + "/" + view.file;
            if (!writeTextFile(path, render.text)) {
                std::cerr << "edi_recipe_ops: cannot write " << path << "\n";
                return 1;
            }
        }
    }

    // Optionally emit the compiled stream (the artifact the Blender driver
    // reads), through the byte-stable writer.
    if (!compiledOut.empty()) {
        RecipeOpStream compiledStream;
        compiledStream.id = stream.id;
        compiledStream.name = stream.name;
        compiledStream.ops = compiled.ops;
        const OpStreamTextResult written = recipeOpsToToml(compiledStream);
        if (!written.ok) {
            std::cerr << "compiled-out refused: " << written.message << "\n";
            return 1;
        }
        if (!writeTextFile(compiledOut, written.text)) {
            std::cerr << "edi_recipe_ops: cannot write " << compiledOut << "\n";
            return 1;
        }
    }

    // The report (stdout): greppable, like edi_plot_job_report.
    std::cout << "edi_recipe_ops\n";
    std::cout << "status: ok\n";
    std::cout << "ops: " << stream.ops.size() << "\n";
    std::cout << "compiled_ops: " << compiled.ops.size() << "\n";
    std::cout << "warnings: " << report.findings.size() << "\n";
    for (const OpFinding &finding : report.findings) {
        std::cout << "- " << finding.code << ": " << finding.message << "\n";
    }
    if (!previewsDir.empty()) {
        std::cout << "previews: " << previewsDir << "\n";
    }
    if (!compiledOut.empty()) {
        std::cout << "compiled_out: " << compiledOut << "\n";
    }
    return 0;
}
