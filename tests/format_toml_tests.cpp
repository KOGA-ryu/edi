#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"

#include <cassert>

using namespace edi::formats;

int main()
{
    auto read = readTomlStaticConfig("name = \"EDI\"\nmode = \"drafting\"\n", "fixture");
    assert(read.ok);
    assert(read.value);
    assert(read.value->at("name") == "EDI");

    auto write = writeTomlStaticConfig(*read.value, "fixture");
    assert(write.ok);
    assert(write.value);
    assert(write.value->find("mode = \"drafting\"") != std::string::npos);

    auto malformed = readTomlStaticConfig("not valid", "fixture");
    assert(!malformed.ok);

    return 0;
}
