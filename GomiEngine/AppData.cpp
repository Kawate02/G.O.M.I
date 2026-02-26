#include "AppData.h"

std::filesystem::path getSavePath()
{
    PWSTR raw = nullptr;

    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw);

    std::filesystem::path dir(raw);
    CoTaskMemFree(raw);

    dir /= "GomiShooting";

    std::filesystem::create_directories(dir);

    return dir / "save.bin";
}