#pragma once

#include <string>

#include "util/alias.h"
#include "util/assert.h"

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

struct IsqDate {
    i16 year;
    i8 month;
    i8 day;

    i16 operator[](ui8 ind) const {
        switch (ind) {
            case 0:
                return year;
            case 1:
                return static_cast<i16>(month);
            case 2:
                return static_cast<i16>(day);
        }
        SEND_MESSAGE("There are not that many numbers in the Date");
    }

    auto operator<=>(const IsqDate&) const = default;
};

struct IsqDatetime {
    i16 year;
    i8 month, day, hour, minute, second;

    i16 operator[](ui8 ind) const {
        switch (ind) {
            case 0:
                return year;
            case 1:
                return static_cast<i16>(month);
            case 2:
                return static_cast<i16>(day);
            case 3:
                return static_cast<i16>(hour);
            case 4:
                return static_cast<i16>(minute);
            case 5:
                return static_cast<i16>(second);
        }
        SEND_MESSAGE("There are not that many numbers in the Datetime");
    }

    auto operator<=>(const IsqDatetime&) const = default;
};

ColType StrToColType(std::string_view s);
std::string ColTypeToStr(ColType t);
