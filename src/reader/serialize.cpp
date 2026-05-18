#include "serialize.h"

std::vector<char> SerializeColumn(const VString &vals, ColType type) {
    const size_t n = vals.size();
    switch (type) {
        case ColType::kInt8:
            return SerializeInts<i8>(vals);
        case ColType::kInt16:
            return SerializeInts<i16>(vals);
        case ColType::kInt32:
            return SerializeInts<i32>(vals);
        case ColType::kInt64:
            return SerializeInts<i64>(vals);
        case ColType::kDate: {
            std::vector<char> out(n * sizeof(ISQDate));
            for (size_t i = 0; i < n; i++) {
                ISQDate date{};
                const auto &s = vals[i];
                if (s.size() >= 10) {
                    std::from_chars(s.data(), s.data() + 4, date.year);
                    std::from_chars(s.data() + 5, s.data() + 7, date.month);
                    std::from_chars(s.data() + 8, s.data() + 10, date.day);
                }
                std::memcpy(out.data() + i * sizeof(ISQDate), &date,
                            sizeof(ISQDate));
            }
            return out;
        }
        case ColType::kDatetime: {
            std::vector<char> out(n * sizeof(ISQDatetime));
            for (size_t i = 0; i < n; i++) {
                ISQDatetime datetime{};
                const auto &s = vals[i];
                if (s.size() >= 10) {
                    std::from_chars(s.data(), s.data() + 4, datetime.year);
                    std::from_chars(s.data() + 5, s.data() + 7, datetime.month);
                    std::from_chars(s.data() + 8, s.data() + 10, datetime.day);
                }
                if (s.size() >= 19) {
                    std::from_chars(s.data() + 11, s.data() + 13,
                                    datetime.hour);
                    std::from_chars(s.data() + 14, s.data() + 16,
                                    datetime.minute);
                    std::from_chars(s.data() + 17, s.data() + 19,
                                    datetime.second);
                }
                std::memcpy(out.data() + i * sizeof(ISQDatetime), &datetime,
                            sizeof(ISQDatetime));
            }
            return out;
        }
        case ColType::kString: {
            std::vector<char> chars;
            std::vector<ui64> offsets(n + 1);
            for (size_t i = 0; i < n; i++) {
                offsets[i] = chars.size();
                chars.insert(chars.end(), vals[i].begin(), vals[i].end());
            }
            offsets[n] = chars.size();
            std::vector<char> out;
            out.reserve(chars.size() + (n + 1) * sizeof(ui64));
            out.insert(out.end(), chars.begin(), chars.end());
            const char *offsets_char =
                reinterpret_cast<const char *>(offsets.data());
            out.insert(out.end(), offsets_char,
                       offsets_char + (n + 1) * sizeof(ui64));
            return out;
        }
    }
    return {};
}