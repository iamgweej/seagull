#pragma once

// msvc++
#include <string>

// 3rd party
#include "include/wil/resource.h"

namespace seagull
{
namespace oleutils
{
std::wstring clsid_to_wstring(REFIID riid)
{
    wil::unique_cotaskmem pOleStr;
    THROW_IF_FAILED(
        ::StringFromCLSID(riid, reinterpret_cast<LPOLESTR *>(&pOleStr)));
    return std::wstring(reinterpret_cast<const wchar_t *>(pOleStr.get()));
}
} // namespace oleutils
} // namespace seagull