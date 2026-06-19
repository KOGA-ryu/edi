#include "io/AssetZooStore.h"

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include <cassert>

using namespace edi::io;
using edi::zoo::AssetRecord;
using edi::zoo::AssetZoo;

int main()
{
    // Round-trip: a populated catalog survives save -> load field-for-field.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("edi-zoo.edizoo"));

        AssetZoo zoo;
        AssetRecord wall;
        wall.id = "asset_0001";
        wall.name = "crypt_wall";
        wall.category = "wall";
        wall.meshRef = "blender:crypt_wall";
        wall.proxyRef = "block_wall_a";
        wall.curated = true;
        wall.textureRefs = {"tex.stone", "tex.moss"};

        AssetRecord door;
        door.id = "asset_0002";
        door.name = "iron_door";
        door.category = "door";
        door.meshRef = "blender:iron_door";
        door.proxyRef = ""; // empty proxy is a valid state
        door.curated = false;
        door.textureRefs = {"tex.iron"};

        zoo.assets = {wall, door};

        const auto saved = saveAssetZoo(path, zoo);
        assert(saved.ok);

        const auto loaded = loadAssetZoo(path);
        assert(loaded.ok);
        assert(loaded.value.has_value());
        const AssetZoo &back = *loaded.value;
        assert(back.assets.size() == 2);

        const auto eq = [](const AssetRecord &a, const AssetRecord &b) {
            return a.id == b.id && a.name == b.name && a.category == b.category
                && a.meshRef == b.meshRef && a.proxyRef == b.proxyRef && a.curated == b.curated
                && a.textureRefs == b.textureRefs;
        };
        assert(eq(back.assets[0], wall));
        assert(eq(back.assets[1], door));
    }

    // Missing file => empty zoo + ok ("no catalog yet" baseline).
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString missing = tempDir.filePath(QStringLiteral("nope.edizoo"));
        const auto loaded = loadAssetZoo(missing);
        assert(loaded.ok);
        assert(loaded.value.has_value());
        assert(loaded.value->assets.empty());
    }

    // Zero-byte EXISTING file => decode error (exists-but-empty == corruption).
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("empty.edizoo"));
        {
            QFile file(path);
            assert(file.open(QIODevice::WriteOnly));
            file.close(); // wrote zero bytes
        }
        const auto loaded = loadAssetZoo(path);
        assert(!loaded.ok);
        assert(loaded.code == edi::formats::FormatResultCode::EmptyBuffer);
    }

    // Corrupt (bad magic) existing file => decode error.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("junk.edizoo"));
        {
            QFile file(path);
            assert(file.open(QIODevice::WriteOnly));
            const QByteArray junk("\x01\x02\x03\x04\x05\x06", 6); // >=5 bytes, wrong magic
            file.write(junk);
            file.close();
        }
        const auto loaded = loadAssetZoo(path);
        assert(!loaded.ok);
    }

    // mkpath: saving into a not-yet-existing subdir creates the parent.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path =
            tempDir.filePath(QStringLiteral("nested/catalog/edi-zoo.edizoo"));
        AssetZoo zoo;
        AssetRecord rec;
        rec.id = "asset_0001";
        rec.name = "floor_tile";
        zoo.assets = {rec};

        const auto saved = saveAssetZoo(path, zoo);
        assert(saved.ok);
        const auto loaded = loadAssetZoo(path);
        assert(loaded.ok);
        assert(loaded.value->assets.size() == 1);
        assert(loaded.value->assets[0].id == "asset_0001");
    }

    return 0;
}
