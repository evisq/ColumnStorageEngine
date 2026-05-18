#pragma once

#include <charconv>
#include <cstring>

#include "column/col_util.h"
#include "util/alias.h"

template <typename T>
std::vector<char> SerializeInts(const VString &vals) {
    std::vector<char> out(vals.size() * sizeof(T));
    for (size_t i = 0; i < vals.size(); i++) {
        T v = 0;
        std::from_chars(vals[i].data(), vals[i].data() + vals[i].size(), v);
        std::memcpy(out.data() + i * sizeof(T), &v, sizeof(T));
    }
    return out;
}

std::vector<char> SerializeColumn(const VString &vals, ColType type);
