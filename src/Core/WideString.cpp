#include <Core/WideString.h>
#include <Core/StringUtils.h>

#include <stdlib.h>
#include <string.h>

namespace Vortex {

// Portable UTF-8 <-> wchar_t conversion.
// wchar_t is 4 bytes (UTF-32) on macOS/Linux, 2 bytes (UTF-16) on Windows.
// This implementation handles both cases.

std::string Narrow(const wchar_t* s, int len) {
    if (s == nullptr || len <= 0) return std::string();
    std::string out;
    out.reserve(len);
    for (int i = 0; i < len; ++i) {
        uint32_t cp;
        if constexpr (sizeof(wchar_t) == 2) {
            // UTF-16: handle surrogate pairs
            uint16_t w = static_cast<uint16_t>(s[i]);
            if (w >= 0xD800 && w <= 0xDBFF && i + 1 < len) {
                uint16_t w2 = static_cast<uint16_t>(s[i + 1]);
                if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                    cp = 0x10000 + ((static_cast<uint32_t>(w - 0xD800) << 10) |
                                    (w2 - 0xDC00));
                    ++i;
                } else {
                    cp = w;
                }
            } else {
                cp = w;
            }
        } else {
            // UTF-32: direct code point
            cp = static_cast<uint32_t>(s[i]);
        }
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
        if constexpr (sizeof(wchar_t) == 2) {
            // UTF-16: encode surrogate pairs for code points above U+FFFF
            if (cp >= 0x10000 && cp < 0x110000) {
                cp -= 0x10000;
                out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
                out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
            } else {
                out.push_back(static_cast<wchar_t>(cp));
            }
        } else {
            out.push_back(static_cast<wchar_t>(cp));
        }
    }
    return out;
}

std::string Narrow(const wchar_t* s) { return Narrow(s, wcslen(s)); }

std::string Narrow(const std::wstring& s) {
    return Narrow(s.c_str(), s.size());
}

std::wstring Widen(const char* s) { return Widen(s, strlen(s)); }

std::wstring Widen(const std::string& s) { return Widen(s.data(), s.length()); }

};  // namespace Vortex
