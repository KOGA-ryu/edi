#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"

#include <cassert>
#include <string>

using namespace edi::formats;

int main()
{
    auto read = readTomlStaticConfig("name = \"EDI\"\nmode = \"drafting\"\n", "fixture");
    assert(read.ok);
    assert(read.value);
    assert(read.errors.empty());
    assert(read.value->at("name") == "EDI");
    assert(formatResultCodeName(FormatResultCode::DuplicateKey) == std::string("duplicate_key"));

    auto write = writeTomlStaticConfig(*read.value, "fixture");
    assert(write.ok);
    assert(write.value);
    assert(write.value->find("mode = \"drafting\"") != std::string::npos);

    auto malformed = readTomlStaticConfig("not valid", "fixture");
    assert(!malformed.ok);
    assert(malformed.errors.front().source == "fixture");
    assert(malformed.errors.front().code == FormatResultCode::SyntaxError);

    auto unquoted = readTomlStaticConfig("name = EDI\n", "fixture");
    assert(!unquoted.ok);
    assert(unquoted.errors.front().code == FormatResultCode::SyntaxError);

    auto emptyKey = readTomlStaticConfig(" = \"EDI\"\n", "fixture");
    assert(!emptyKey.ok);
    assert(emptyKey.errors.front().code == FormatResultCode::EmptyKey);

    auto duplicate = readTomlStaticConfig("name = \"EDI\"\nname = \"Again\"\n", "fixture");
    assert(!duplicate.ok);
    assert(duplicate.errors.front().code == FormatResultCode::DuplicateKey);

    StaticConfig invalid;
    invalid[""] = "bad";
    auto badWrite = writeTomlStaticConfig(invalid, "fixture");
    assert(!badWrite.ok);
    assert(badWrite.errors.front().code == FormatResultCode::EmptyKey);

    return 0;
}
