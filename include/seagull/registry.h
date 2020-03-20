#pragma once

// msvc++
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <numeric>

// 3rd party
#include "include/wil/resource.h"

namespace seagull
{

/**
 * registry
 * 
 * Wraps access to the windows registry.
 */
namespace registry
{

struct Value
{
    DWORD type;
    std::unique_ptr<BYTE[]> data;
    DWORD length;
};

class Key
{
public:
    // C'tors.
    // Construct from raw `HKEY`.
    // These do not take ownership of the `hKeyParent` param.
    Key() noexcept = default;
    Key(HKEY hKeyParent, const std::wstring &subKey);
    Key(HKEY hKeyParent, const std::wstring &subKey, REGSAM desiredAccess);

    // Get the raw `HKEY`
    HKEY get() const noexcept;

    // Check if the Key is valid.
    bool is_valid() const noexcept;
    explicit operator bool() const noexcept;

    // Thin wrappers
    void create(HKEY hKeyParent,
                const std::wstring &subKey,
                REGSAM desiredAccess = KEY_READ | KEY_WRITE);

    void create(HKEY hKeyParent,
                const std::wstring &subKey,
                REGSAM desiredAccess,
                DWORD options,
                LPSECURITY_ATTRIBUTES securityAttributes,
                LPDWORD disposition);

    void open(HKEY hKeyParent,
              const std::wstring &subKey,
              REGSAM desiredAccess = KEY_READ | KEY_WRITE);

    // value setters
    void set_dword_value(const std::wstring &valueName, DWORD val);
    void set_qword_value(const std::wstring &valueName, ULONGLONG val);
    void set_string_value(const std::wstring &valueName, const std::wstring &val);
    void set_multistring_value(const std::wstring &valueName, const std::vector<std::wstring> &val);
    void set_expanded_string_value(const std::wstring &valueName, const std::wstring &val);
    void set_binary_value(const std::wstring &valueName, LPCVOID val, DWORD size);
    void set_binary_value(const std::wstring &valueName, std::vector<BYTE> val);

    // vector enumerators
    std::vector<std::wstring> enum_subkeys() const;
    std::vector<std::pair<std::wstring, Value>> enum_values() const;
    std::vector<std::wstring> enum_value_names() const;

    // generic registry functions
    void delete_value(const std::wstring &valueName);
    void delete_key(const std::wstring &subKey, REGSAM desiredAccess);
    void delete_tree(const std::wstring &subKey);

private:
    wil::unique_hkey m_hKey;
};

namespace internals
{
std::vector<wchar_t> build_multistring(const std::vector<std::wstring> strings)
{
    if (strings.empty())
    {
        return std::vector<wchar_t>(L'\0', 2);
    }

    DWORD totalLength = 1; // final NUL-terminator
    for (const auto &s : strings)
    {
        totalLength += (s.size() + 1);
    }

    std::vector<wchar_t> ret;
    ret.reserve(totalLength);

    for (const auto &s : strings)
    {
        ret.insert(ret.end(), s.begin(), s.end());
        ret.push_back(L'\0');
    }
    ret.push_back(L'\0');

    return ret;
}
} // namespace internals

// ++++++++++++++++++
// ++ Constructors ++
// ++++++++++++++++++

Key::Key(HKEY hKeyParent, const std::wstring &subKey)
{
    create(hKeyParent, subKey);
}

Key::Key(HKEY hKeyParent, const std::wstring &subKey, REGSAM desiredAccess)
{
    create(hKeyParent, subKey, desiredAccess);
}

// ++++++++++++++++++++++++++++
// ++ Simple RAII operations ++
// ++++++++++++++++++++++++++++

inline HKEY Key::get() const noexcept
{
    return m_hKey.get();
}

inline bool Key::is_valid() const noexcept
{
    return m_hKey.is_valid();
}

Key::operator bool() const noexcept
{
    return is_valid();
}

// +++++++++++++++++++
// ++ Thin Wrappers ++
// +++++++++++++++++++

inline void Key::create(HKEY hKeyParent,
                        const std::wstring &subKey,
                        REGSAM desiredAccess)
{
    create(
        hKeyParent,
        subKey,
        desiredAccess,
        REG_OPTION_NON_VOLATILE,
        nullptr,
        nullptr);
}

inline void Key::create(HKEY hKeyParent,
                        const std::wstring &subKey,
                        REGSAM desiredAccess,
                        DWORD options,
                        LPSECURITY_ATTRIBUTES secuirtyAttributes,
                        LPDWORD disposition)
{
    HKEY hKey;
    THROW_IF_WIN32_ERROR(::RegCreateKeyExW(
        hKeyParent,
        subKey.c_str(),
        0,
        REG_NONE,
        options,
        desiredAccess,
        secuirtyAttributes,
        &hKey,
        disposition));
    *(&m_hKey) = hKey;
}

inline void Key::open(HKEY hKeyParent,
                      const std::wstring &subKey,
                      REGSAM desiredAccess)
{
    HKEY hKey;
    THROW_IF_WIN32_ERROR(::RegOpenKeyExW(
        hKeyParent,
        subKey.c_str(),
        REG_NONE,
        desiredAccess,
        &hKey));
    *(&m_hKey) = hKey;
}

// +++++++++++++++++++
// ++ Value Setters ++
// +++++++++++++++++++

inline void Key::set_dword_value(const std::wstring &valueName, DWORD val)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_DWORD,
            reinterpret_cast<LPCBYTE>(&val),
            sizeof(val)));
}

inline void Key::set_qword_value(const std::wstring &valueName, ULONGLONG val)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_QWORD,
            reinterpret_cast<LPCBYTE>(&val),
            sizeof(val)));
}

inline void Key::set_string_value(const std::wstring &valueName, const std::wstring &val)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_SZ,
            reinterpret_cast<LPCBYTE>(val.c_str()),
            static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t)) // Accout for NUL-terminator
            ));
}

inline void Key::set_multistring_value(const std::wstring &valueName, const std::vector<std::wstring> &val)
{
    auto buf = internals::build_multistring(val);
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_MULTI_SZ,
            reinterpret_cast<LPCBYTE>(buf.data()),
            static_cast<DWORD>(buf.size() * sizeof(wchar_t))));
}

inline void Key::set_expanded_string_value(const std::wstring &valueName, const std::wstring &val)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_EXPAND_SZ,
            reinterpret_cast<LPCBYTE>(val.c_str()),
            static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t))));
}

inline void Key::set_binary_value(const std::wstring &valueName, LPCVOID val, DWORD size)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_BINARY,
            reinterpret_cast<LPCBYTE>(val),
            size));
}

inline void Key::set_binary_value(const std::wstring &valueName, std::vector<BYTE> val)
{
    THROW_IF_WIN32_ERROR(
        ::RegSetValueExW(
            m_hKey.get(),
            valueName.c_str(),
            0,
            REG_BINARY,
            val.data(),
            static_cast<DWORD>(val.size())));
}

// ++++++++++++++++++++++++
// ++ Vector Enumerators ++
// ++++++++++++++++++++++++

inline std::vector<std::wstring> Key::enum_subkeys() const
{
    DWORD subKeyCount;
    DWORD maxSubKeyNameLength;

    THROW_IF_WIN32_ERROR(::RegQueryInfoKeyW(
        m_hKey.get(),
        nullptr,
        nullptr,
        nullptr,
        &subKeyCount,
        &maxSubKeyNameLength,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr));

    maxSubKeyNameLength++;
    std::vector<std::wstring> ret;
    auto buf = std::make_unique<WCHAR[]>(maxSubKeyNameLength);

    for (DWORD i = 0; i < subKeyCount; ++i)
    {
        DWORD subKeyNameLength = maxSubKeyNameLength;
        THROW_IF_WIN32_ERROR(::RegEnumKeyExW(
            m_hKey.get(),
            i,
            buf.get(),
            &subKeyNameLength,
            nullptr,
            nullptr,
            nullptr,
            nullptr));
        ret.push_back(std::wstring(buf.get(), subKeyNameLength));
    }

    return ret;
}

inline std::vector<std::pair<std::wstring, Value>> Key::enum_values() const
{
    DWORD valCount;
    DWORD maxValNameLength;
    DWORD maxValLength;

    THROW_IF_WIN32_ERROR(::RegQueryInfoKeyW(
        m_hKey.get(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valCount,
        &maxValNameLength,
        &maxValLength,
        nullptr,
        nullptr));

    maxValNameLength++;
    std::vector<std::pair<std::wstring, Value>> ret;
    auto nameBuf = std::make_unique<WCHAR[]>(maxValNameLength);
    auto valBuf = std::make_unique<BYTE[]>(maxValLength);

    for (DWORD i = 0; i < valCount; ++i)
    {
        DWORD valNameLength = maxValNameLength;
        DWORD valLength = maxValLength;
        DWORD type;

        THROW_IF_WIN32_ERROR(::RegEnumValueW(
            m_hKey.get(),
            i,
            nameBuf.get(),
            &valNameLength,
            nullptr,
            &type,
            valBuf.get(),
            &valLength));

        auto valBufOther = std::make_unique<BYTE[]>(valLength);
        std::memcpy(valBufOther.get(), valBuf.get(), valLength);

        ret.push_back(std::make_pair(
            std::wstring(nameBuf.get(), valNameLength),
            Value{
                type,
                std::move(valBufOther),
                valLength}));
    }

    return ret;
}

inline std::vector<std::wstring> Key::enum_value_names() const
{
    DWORD valCount;
    DWORD maxValNameLength;

    THROW_IF_WIN32_ERROR(::RegQueryInfoKeyW(
        m_hKey.get(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valCount,
        &maxValNameLength,
        nullptr,
        nullptr,
        nullptr));

    maxValNameLength++;
    std::vector<std::wstring> ret;
    auto nameBuf = std::make_unique<WCHAR[]>(maxValNameLength);

    for (DWORD i = 0; i < valCount; ++i)
    {
        DWORD valNameLength = maxValNameLength;

        THROW_IF_WIN32_ERROR(::RegEnumValueW(
            m_hKey.get(),
            i,
            nameBuf.get(),
            &valNameLength,
            nullptr,
            NULL,
            NULL,
            NULL));

        ret.push_back(std::wstring(nameBuf.get(), valNameLength));
    }

    return ret;
}

// ++++++++++++++++++++++++++++++++
// ++ Generic Registry Functions ++
// ++++++++++++++++++++++++++++++++

inline void Key::delete_value(const std::wstring &valueName)
{
    THROW_IF_WIN32_ERROR(
        ::RegDeleteValueW(
            m_hKey.get(),
            valueName.c_str()));
}

inline void Key::delete_key(const std::wstring &subKey, REGSAM desiredAccess)
{
    THROW_IF_WIN32_ERROR(
        ::RegDeleteKeyExW(
            m_hKey.get(),
            subKey.c_str(),
            desiredAccess,
            0));
}

inline void Key::delete_tree(const std::wstring &subKey)
{
    THROW_IF_WIN32_ERROR(
        ::RegDeleteTreeW(
            m_hKey.get(),
            subKey.c_str()));
}

} // namespace registry

} // namespace seagull
