#pragma once

#include <fstream>

template <typename T>
static void Put(std::ofstream &out, T v) {
    out.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

template <typename T>
static T Get(std::ifstream &in) {
    T v{};
    in.read(reinterpret_cast<char *>(&v), sizeof(v));
    return v;
}
