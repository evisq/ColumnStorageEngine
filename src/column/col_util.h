#pragma once

#include <string>

#include "util/alias.h"

enum class ColType : ui8 {
    kInt8 = 1,
    kInt16 = 2,
    kInt32 = 3,
    kInt64 = 4,
    kString = 5,
    kDate = 6,
    kDatetime = 7,
};

struct ColScheme {
    std::string name;
    ColType type;
};

struct ISQDate {
    i16 year;
    i8 month;
    i8 day;

    auto operator<=>(const ISQDate&) const = default;
};

struct ISQDatetime {
    i16 year;
    i8 month, day, hour, minute, second;

    auto operator<=>(const ISQDatetime&) const = default;
};

ColType StrToColType(std::string_view s);
std::string ColTypeToStr(ColType t);
