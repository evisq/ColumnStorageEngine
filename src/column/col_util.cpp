#include "column/col_util.h"

ColType StrToColType(std::string_view s) {
    if (s == "int8") return ColType::kInt8;
    if (s == "int16") return ColType::kInt16;
    if (s == "int32") return ColType::kInt32;
    if (s == "int64") return ColType::kInt64;
    if (s == "string") return ColType::kString;
    if (s == "date") return ColType::kDate;
    if (s == "datetime" || s == "timestamp") return ColType::kDatetime;
    SEND_MESSAGE("Unknown column type: " + std::string(s));
}

std::string ColTypeToStr(ColType t) {
    if (t == ColType::kInt8) return "int8";
    if (t == ColType::kInt16) return "int16";
    if (t == ColType::kInt32) return "int32";
    if (t == ColType::kInt64) return "int64";
    if (t == ColType::kString) return "string";
    if (t == ColType::kDate) return "date";
    if (t == ColType::kDatetime) return "datetime";
    return "unknown";
}
