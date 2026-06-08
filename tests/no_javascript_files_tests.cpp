#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool isJavaScriptExtension(const std::filesystem::path &path)
{
    const auto extension = path.extension().string();
    return extension == ".js" || extension == ".mjs" || extension == ".cjs"
        || extension == ".jsx" || extension == ".ts" || extension == ".tsx"
        || extension == ".qml";
}

bool shouldSkipDirectory(const std::filesystem::path &path)
{
    const auto filename = path.filename().string();
    return filename == ".git" || filename == "build";
}

} // namespace

int main()
{
    const std::filesystem::path root{PROJECT_SOURCE_DIR};
    std::vector<std::filesystem::path> matches;

    std::filesystem::recursive_directory_iterator it{
        root,
        std::filesystem::directory_options::skip_permission_denied
    };
    const std::filesystem::recursive_directory_iterator end;

    for (; it != end; ++it) {
        const auto &entry = *it;
        if (entry.is_directory() && shouldSkipDirectory(entry.path())) {
            it.disable_recursion_pending();
            continue;
        }
        if (entry.is_regular_file() && isJavaScriptExtension(entry.path())) {
            matches.push_back(std::filesystem::relative(entry.path(), root));
        }
    }

    std::sort(matches.begin(), matches.end());
    for (const auto &match : matches) {
        std::cerr << match.string() << '\n';
    }

    if (!matches.empty()) {
        std::cerr << "Found " << matches.size() << " JavaScript/QML file(s).\n";
        return 1;
    }

    return 0;
}
