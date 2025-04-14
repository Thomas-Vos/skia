/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/base/SkStringView.h"
#include "src/sksl/SkSLDefines.h"
#include "src/sksl/SkSLString.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

template <typename RoundtripType, int kFullPrecision>
static std::string to_string_impl(RoundtripType value) {
    char buffer[64];
    int len;

    if constexpr (std::is_same_v<RoundtripType, float>) {
        len = std::snprintf(buffer, sizeof(buffer), "%.*g", kFullPrecision, value);
    } else {
        len = std::snprintf(buffer, sizeof(buffer), "%.*g", kFullPrecision, value);
    }

    std::string text(buffer, len);

    // Ensure it has a decimal point to differentiate from integers
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos) {
        text += ".0";
    }

    return text;
}

std::string skstd::to_string(float value) {
    return to_string_impl<float, 9>(value);
}

std::string skstd::to_string(double value) {
    return to_string_impl<double, 17>(value);
}

bool SkSL::stod(std::string_view s, SKSL_FLOAT* value) {
    if (!value || s.empty()) {
        return false;
    }

    // Copy to null-terminated buffer (needed for std::strtof)
    std::string str(s.data(), s.size());
    char* end = nullptr;
    const char* start = str.c_str();

    // Use std::strtof (C-style, no locale issues)
    float parsed = std::strtof(start, &end);

    // Check if any characters were parsed and the result is finite
    if (start == end || !std::isfinite(parsed)) {
        return false;
    }

    *value = parsed;
    return true;
}

bool SkSL::stoi(std::string_view s, SKSL_INT* value) {
    if (s.empty()) {
        return false;
    }
    char suffix = s.back();
    if (suffix == 'u' || suffix == 'U') {
        s.remove_suffix(1);
    }
    std::string str(s);  // s is not null-terminated
    const char* strEnd = str.data() + str.length();
    char* p;
    errno = 0;
    unsigned long long result = strtoull(str.data(), &p, /*base=*/0);
    *value = static_cast<SKSL_INT>(result);
    return p == strEnd && errno == 0 && result <= 0xFFFFFFFF;
}

std::string SkSL::String::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string result;
    vappendf(&result, fmt, args);
    va_end(args);
    return result;
}

void SkSL::String::appendf(std::string *str, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vappendf(str, fmt, args);
    va_end(args);
}

void SkSL::String::vappendf(std::string *str, const char* fmt, va_list args) {
    #define BUFFER_SIZE 256
    char buffer[BUFFER_SIZE];
    va_list reuse;
    va_copy(reuse, args);
    size_t size = vsnprintf(buffer, BUFFER_SIZE, fmt, args);
    if (BUFFER_SIZE >= size + 1) {
        str->append(buffer, size);
    } else {
        auto newBuffer = std::unique_ptr<char[]>(new char[size + 1]);
        vsnprintf(newBuffer.get(), size + 1, fmt, reuse);
        str->append(newBuffer.get(), size);
    }
    va_end(reuse);
}
