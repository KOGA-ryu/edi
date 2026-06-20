#include "drafting/DraftingTypes.h"

#include "EdiAssert.h"
#include <cstddef>
#include <string>
#include <utility>
#include <variant>

using namespace edi::drafting;

// One check per DraftingGeometry alternative, derived from the variant ITSELF.
// `shapeKindOf<G>()` is the compile-time bridge from a geometry type to its
// DraftingShapeKind; this asserts that every kind has a real name and that the
// name round-trips back to the same kind.
template <std::size_t Index>
void checkAlternative()
{
    using G = std::variant_alternative_t<Index, DraftingGeometry>;
    constexpr DraftingShapeKind kind = shapeKindOf<G>();

    const char *name = shapeKindName(kind);
    EDI_CHECK(name != nullptr);
    // -Werror=switch guards shapeKindName at compile time; this catches the
    // belt-and-suspenders "unknown" fallback ever being reached for a real kind.
    EDI_CHECK(std::string(name) != "unknown");

    // The string maps (shapeKindFromName) are NOT switches — the compiler can't
    // guard them. This is where a forgotten entry shows up: a kind that names
    // itself but can't be read back would fail to round-trip here, instead of
    // silently failing to load a saved .edidraw.
    EDI_CHECK(shapeKindFromName(name) == kind);
}

template <std::size_t... Index>
void checkAll(std::index_sequence<Index...>)
{
    (checkAlternative<Index>(), ...);
}

int main()
{
    // Coverage by construction: adding a DraftingGeometry alternative adds its
    // check here automatically (the index sequence spans the whole variant), so
    // a forgotten name-map entry fails THIS test rather than hiding in a file.
    checkAll(std::make_index_sequence<std::variant_size_v<DraftingGeometry>>{});
    return 0;
}
