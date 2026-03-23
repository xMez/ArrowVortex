#include <Core/WideString.h>
#include <Core/StringUtils.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

namespace Vortex {

#ifdef _WIN32

std::string Narrow(const wchar_t* s, int len) {
    if (s == nullptr || len <= 0) return std::string();
    auto new_size =
        WideCharToMultiByte(CP_UTF8, 0, s, len, nullptr, 0, nullptr, nullptr);
    std::string out(new_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, s, len, out.data(), out.size(), nullptr,
                        nullptr);
    return out;
}

std::wstring Widen(const char* s, int len) {
    std::wstring out;
    if (s == nullptr || len <= 0) return out;
    auto new_size = MultiByteToWideChar(CP_UTF8, 0, s, len, nullptr, 0);
    std::wstring out_str(new_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s, len, &out_str[0], new_size);
    out.assign(out_str.data(), out_str.data() + out_str.size());
    return out;
}

#else  // macOS/Linux: wchar_t is UTF-32

std::string Narrow(const wchar_t* s, int len) {
    if (s == nullptr || len <= 0) return std::string();
    std::string out;
    out.reserve(len);
    for (int i = 0; i < len; ++i) {
        uint32_t cp = static_cast<uint32_t>(s[i]);
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x110000) {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

std::wstring Widen(const char* s, int len) {
    if (s == nullptr || len <= 0) return std::wstring();
    std::wstring out;
    out.reserve(len);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    const unsigned char* end = p + len;
    while (p < end) {
        uint32_t cp;
        if (*p < 0x80) {
            cp = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            cp = (*p++ & 0x1F) << 6;
            if (p < end) cp |= (*p++ & 0x3F);
        } else if ((*p & 0xF0) == 0xE0) {
            cp = (*p++ & 0x0F) << 12;
            if (p < end) cp |= (*p++ & 0x3F) << 6;
            if (p < end) cp |= (*p++ & 0x3F);
        } else if ((*p & 0xF8) == 0xF0) {
            cp = (*p++ & 0x07) << 18;
            if (p < end) cp |= (*p++ & 0x3F) << 12;
            if (p < end) cp |= (*p++ & 0x3F) << 6;
            if (p < end) cp |= (*p++ & 0x3F);
        } else {
            ++p;  // skip invalid byte
            continue;
        }
        out.push_back(static_cast<wchar_t>(cp));
    }
    return out;
}

#endif  // _WIN32

std::string Narrow(const wchar_t* s) { return Narrow(s, wcslen(s)); }

std::string Narrow(const std::wstring& s) {
    return Narrow(s.c_str(), s.size());
}

std::wstring Widen(const char* s) { return Widen(s, strlen(s)); }

std::wstring Widen(const std::string& s) { return Widen(s.data(), s.length()); }

};  // namespace Vortex

#endif
