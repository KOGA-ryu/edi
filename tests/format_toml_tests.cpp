#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"

#include "EdiAssert.h"
#include <string>

using namespace edi::formats;

int main()
{
    auto read = readTomlStaticConfig("name = \"EDI\"\nmode = \"drafting\"\n", "fixture");
    EDI_CHECK(read.ok);
    EDI_CHECK(read.value);
    EDI_CHECK(read.errors.empty());
    EDI_CHECK(read.value->at("name") == "EDI");
    EDI_CHECK(formatResultCodeName(FormatResultCode::DuplicateKey) == std::string("duplicate_key"));

    auto write = writeTomlStaticConfig(*read.value, "fixture");
    EDI_CHECK(write.ok);
    EDI_CHECK(write.value);
    EDI_CHECK(write.value->find("mode = \"drafting\"") != std::string::npos);

    auto malformed = readTomlStaticConfig("not valid", "fixture");
    EDI_CHECK(!malformed.ok);
    EDI_CHECK(malformed.code == FormatResultCode::SyntaxError);
    EDI_CHECK(malformed.errors.front().source == "fixture");
    EDI_CHECK(malformed.errors.front().code == FormatResultCode::SyntaxError);

    auto unquoted = readTomlStaticConfig("name = EDI\n", "fixture");
    EDI_CHECK(!unquoted.ok);
    EDI_CHECK(unquoted.code == FormatResultCode::SyntaxError);
    EDI_CHECK(unquoted.errors.front().code == FormatResultCode::SyntaxError);

    auto emptyKey = readTomlStaticConfig(" = \"EDI\"\n", "fixture");
    EDI_CHECK(!emptyKey.ok);
    EDI_CHECK(emptyKey.errors.front().code == FormatResultCode::EmptyKey);

    auto duplicate = readTomlStaticConfig("name = \"EDI\"\nname = \"Again\"\n", "fixture");
    EDI_CHECK(!duplicate.ok);
    EDI_CHECK(duplicate.errors.front().code == FormatResultCode::DuplicateKey);

    StaticConfig invalid;
    invalid[""] = "bad";
    auto badWrite = writeTomlStaticConfig(invalid, "fixture");
    EDI_CHECK(!badWrite.ok);
    EDI_CHECK(badWrite.code == FormatResultCode::EmptyKey);
    EDI_CHECK(badWrite.errors.front().code == FormatResultCode::EmptyKey);

    return 0;
}
